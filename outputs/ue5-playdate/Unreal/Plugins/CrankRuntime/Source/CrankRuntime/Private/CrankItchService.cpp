#include "CrankItchService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"
#if PLATFORM_ANDROID
#include "Android/AndroidJavaEnv.h"
#endif

FCrankItchService::FCrankItchService() {
}

FCrankItchService::~FCrankItchService() {
    if (CurrentRequest.IsValid()) {
        CurrentRequest->CancelRequest();
        CurrentRequest.Reset();
    }
}

void FCrankItchService::SetApiKey(const FString& InApiKey) {
    ApiKey = InApiKey.TrimStartAndEnd();
    if (!ApiKey.IsEmpty()) {
        VerifyApiKey(ApiKey);
    } else {
        ClearAccount();
    }
}

void FCrankItchService::ClearAccount() {
    ApiKey.Empty();
    Username.Empty();
    bAuthenticated = false;
    bBusy = false;
    StatusMessage.Empty();
    OwnedGames.Empty();
    if (CurrentRequest.IsValid()) {
        CurrentRequest->CancelRequest();
        CurrentRequest.Reset();
    }
}

void FCrankItchService::VerifyApiKey(const FString& KeyToTest, FOnItchVerifyCompleted OnComplete) {
    const FString Trimmed = KeyToTest.TrimStartAndEnd();
    if (Trimmed.IsEmpty()) {
        StatusMessage = TEXT("API Key cannot be empty.");
        OnComplete.ExecuteIfBound(false, StatusMessage);
        return;
    }

    if (CurrentRequest.IsValid()) {
        CurrentRequest->CancelRequest();
        CurrentRequest.Reset();
    }

    bBusy = true;
    StatusMessage = TEXT("Verifying Itch.io API Key...");

    auto Request = FHttpModule::Get().CreateRequest();
    CurrentRequest = Request;
    Request->SetURL(FString::Printf(TEXT("https://itch.io/api/1/%s/me"), *Trimmed));
    Request->SetVerb(TEXT("GET"));
    Request->SetTimeout(15.f);
    Request->SetHeader(TEXT("User-Agent"), TEXT("CrankLab-Playdate/1.0"));

    Request->OnProcessRequestComplete().BindLambda([this, Trimmed, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        CurrentRequest.Reset();
        bBusy = false;

        if (!bSuccess || !Res.IsValid()) {
            bAuthenticated = false;
            StatusMessage = TEXT("Network request failed. Check internet connection.");
            OnComplete.ExecuteIfBound(false, StatusMessage);
            return;
        }

        const int32 Code = Res->GetResponseCode();
        const FString Content = Res->GetContentAsString();

        TSharedPtr<FJsonObject> Root;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) {
            bAuthenticated = false;
            StatusMessage = TEXT("Invalid response from Itch.io.");
            OnComplete.ExecuteIfBound(false, StatusMessage);
            return;
        }

        const TSharedPtr<FJsonObject>* UserObj = nullptr;
        if (Code == 200 && Root->TryGetObjectField(TEXT("user"), UserObj) && UserObj && (*UserObj)->HasField(TEXT("username"))) {
            ApiKey = Trimmed;
            Username = (*UserObj)->GetStringField(TEXT("username"));
            bAuthenticated = true;
            StatusMessage = FString::Printf(TEXT("Connected as: %s"), *Username);
            OnComplete.ExecuteIfBound(true, Username);
        } else {
            bAuthenticated = false;
            const TArray<TSharedPtr<FJsonValue>>* Errors = nullptr;
            if (Root->TryGetArrayField(TEXT("errors"), Errors) && Errors && Errors->Num() > 0) {
                StatusMessage = (*Errors)[0]->AsString();
            } else {
                StatusMessage = FString::Printf(TEXT("Authentication failed (HTTP %d). Check API Key."), Code);
            }
            OnComplete.ExecuteIfBound(false, StatusMessage);
        }
    });

    Request->ProcessRequest();
}

void FCrankItchService::SyncOwnedLibrary(FOnItchSyncCompleted OnComplete) {
    if (!bAuthenticated || ApiKey.IsEmpty()) {
        StatusMessage = TEXT("Not authenticated. Please verify your API Key first.");
        OnComplete.ExecuteIfBound(false, StatusMessage);
        return;
    }

    if (CurrentRequest.IsValid()) {
        CurrentRequest->CancelRequest();
        CurrentRequest.Reset();
    }

    bBusy = true;
    StatusMessage = TEXT("Syncing library from Itch.io...");

    auto Request = FHttpModule::Get().CreateRequest();
    CurrentRequest = Request;
    Request->SetURL(FString::Printf(TEXT("https://itch.io/api/1/%s/my-owned-keys"), *ApiKey));
    Request->SetVerb(TEXT("GET"));
    Request->SetTimeout(30.f);
    Request->SetHeader(TEXT("User-Agent"), TEXT("CrankLab-Playdate/1.0"));

    Request->OnProcessRequestComplete().BindLambda([this, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        CurrentRequest.Reset();
        bBusy = false;

        if (!bSuccess || !Res.IsValid() || Res->GetResponseCode() != 200) {
            StatusMessage = TEXT("Failed to query library from Itch.io.");
            OnComplete.ExecuteIfBound(false, StatusMessage);
            return;
        }

        TSharedPtr<FJsonObject> Root;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) {
            StatusMessage = TEXT("Failed to parse Itch.io library response.");
            OnComplete.ExecuteIfBound(false, StatusMessage);
            return;
        }

        OwnedGames.Empty();
        const TArray<TSharedPtr<FJsonValue>>* KeysArray = nullptr;
        if (Root->TryGetArrayField(TEXT("owned_keys"), KeysArray) && KeysArray) {
            for (const auto& KeyVal : *KeysArray) {
                auto KeyObj = KeyVal->AsObject();
                if (!KeyObj.IsValid()) continue;

                const TSharedPtr<FJsonObject>* GameObj = nullptr;
                if (!KeyObj->TryGetObjectField(TEXT("game"), GameObj) || !GameObj || !GameObj->IsValid()) continue;

                FItchGameItem Item;
                Item.GameId = (*GameObj)->GetIntegerField(TEXT("id"));
                Item.DownloadKeyId = KeyObj->GetIntegerField(TEXT("id"));
                Item.Title = (*GameObj)->GetStringField(TEXT("title"));
                (*GameObj)->TryGetStringField(TEXT("url"), Item.Url);
                (*GameObj)->TryGetStringField(TEXT("cover_url"), Item.CoverUrl);

                const TSharedPtr<FJsonObject>* UserObj = nullptr;
                if ((*GameObj)->TryGetObjectField(TEXT("user"), UserObj) && UserObj && (*UserObj)->IsValid()) {
                    (*UserObj)->TryGetStringField(TEXT("username"), Item.Author);
                }

                // Check for Playdate tags / classifications or retain item
                bool bIsPlaydate = false;
                (*GameObj)->TryGetBoolField(TEXT("p_playdate"), bIsPlaydate);
                if (!bIsPlaydate && Item.Title.Contains(TEXT("Playdate"), ESearchCase::IgnoreCase)) {
                    bIsPlaydate = true;
                }

                // If flagged or potential, keep in list
                OwnedGames.Add(Item);
            }
        }

        StatusMessage = FString::Printf(TEXT("Synchronized %d games from Itch.io."), OwnedGames.Num());
        OnComplete.ExecuteIfBound(true, StatusMessage);
    });

    Request->ProcessRequest();
}

void FCrankItchService::ResolveDownloadUrl(int64 DownloadKeyId, int64 UploadId, FOnItchDownloadResolved OnComplete) {
    if (!bAuthenticated || ApiKey.IsEmpty()) {
        OnComplete.ExecuteIfBound(false, TEXT("Not authenticated."));
        return;
    }

    auto Request = FHttpModule::Get().CreateRequest();
    FString Url;
    if (DownloadKeyId > 0) {
        Url = FString::Printf(TEXT("https://itch.io/api/1/%s/download-key/%lld/download/%lld"), *ApiKey, DownloadKeyId, UploadId);
    } else {
        Url = FString::Printf(TEXT("https://itch.io/api/1/%s/upload/%lld/download"), *ApiKey, UploadId);
    }

    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetTimeout(20.f);
    Request->SetHeader(TEXT("User-Agent"), TEXT("CrankLab-Playdate/1.0"));

    Request->OnProcessRequestComplete().BindLambda([OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess) {
        if (!bSuccess || !Res.IsValid()) {
            OnComplete.ExecuteIfBound(false, TEXT("Failed to request download URL."));
            return;
        }

        const int32 Code = Res->GetResponseCode();
        if (Code == 302 || Code == 301) {
            const FString Location = Res->GetHeader(TEXT("Location"));
            if (!Location.IsEmpty()) {
                OnComplete.ExecuteIfBound(true, Location);
                return;
            }
        }

        TSharedPtr<FJsonObject> Root;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());
        if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid()) {
            FString DownloadUrl;
            if (Root->TryGetStringField(TEXT("url"), DownloadUrl) && !DownloadUrl.IsEmpty()) {
                OnComplete.ExecuteIfBound(true, DownloadUrl);
                return;
            }
        }

        OnComplete.ExecuteIfBound(false, FString::Printf(TEXT("Download URL resolution failed (HTTP %d)."), Code));
    });

    Request->ProcessRequest();
}

void FCrankItchService::BeginWebLogin() {
#if PLATFORM_ANDROID
    JNIEnv* Env = AndroidJavaEnv::GetJavaEnv();
    if (!Env) return;
    jclass Class = AndroidJavaEnv::FindJavaClass("com.cranklab.CrankItchAuth");
    if (!Class) return;
    jmethodID Method = Env->GetStaticMethodID(Class, "openLogin", "(Landroid/app/Activity;)V");
    if (Method) {
        Env->CallStaticVoidMethod(Class, Method, AndroidJavaEnv::GetGameActivityThis());
        bWebLoginPending = true;
        StatusMessage = TEXT("Browser login opened...");
    }
    if (Env->ExceptionCheck()) Env->ExceptionClear();
    Env->DeleteLocalRef(Class);
#else
    FPlatformProcess::LaunchURL(TEXT("https://itch.io/user/settings/api-keys"), nullptr, nullptr);
    StatusMessage = TEXT("Opened itch.io in browser. Copy your API key, then tap Paste API Key.");
#endif
}

void FCrankItchService::CloseWebLogin() {
#if PLATFORM_ANDROID
    JNIEnv* Env = AndroidJavaEnv::GetJavaEnv();
    if (!Env) return;
    jclass Class = AndroidJavaEnv::FindJavaClass("com.cranklab.CrankItchAuth");
    if (!Class) return;
    jmethodID Method = Env->GetStaticMethodID(Class, "close", "()V");
    if (Method) Env->CallStaticVoidMethod(Class, Method);
    if (Env->ExceptionCheck()) Env->ExceptionClear();
    Env->DeleteLocalRef(Class);
#endif
    bWebLoginPending = false;
}

bool FCrankItchService::IsWebLoginActive() const {
    return bWebLoginPending;
}

void FCrankItchService::PollWebLogin(TFunction<void(bool bSuccess, const FString& KeyOrError)> OnResult) {
#if PLATFORM_ANDROID
    if (!bWebLoginPending) return;
    JNIEnv* Env = AndroidJavaEnv::GetJavaEnv();
    if (!Env) return;
    jclass Class = AndroidJavaEnv::FindJavaClass("com.cranklab.CrankItchAuth");
    if (!Class) return;
    jmethodID Method = Env->GetStaticMethodID(Class, "poll", "()[Ljava/lang/String;");
    auto Array = Method ? static_cast<jobjectArray>(Env->CallStaticObjectMethod(Class, Method)) : nullptr;
    if (Array && Env->GetArrayLength(Array) >= 3) {
        auto S0 = static_cast<jstring>(Env->GetObjectArrayElement(Array, 0));
        auto S1 = static_cast<jstring>(Env->GetObjectArrayElement(Array, 1));
        auto S2 = static_cast<jstring>(Env->GetObjectArrayElement(Array, 2));

        const char* C0 = S0 ? Env->GetStringUTFChars(S0, nullptr) : nullptr;
        const char* C1 = S1 ? Env->GetStringUTFChars(S1, nullptr) : nullptr;
        const char* C2 = S2 ? Env->GetStringUTFChars(S2, nullptr) : nullptr;

        FString StateStr = C0 ? UTF8_TO_TCHAR(C0) : TEXT("0");
        FString KeyStr = C1 ? UTF8_TO_TCHAR(C1) : TEXT("");
        FString MsgStr = C2 ? UTF8_TO_TCHAR(C2) : TEXT("");

        if (C0) Env->ReleaseStringUTFChars(S0, C0);
        if (C1) Env->ReleaseStringUTFChars(S1, C1);
        if (C2) Env->ReleaseStringUTFChars(S2, C2);

        if (S0) Env->DeleteLocalRef(S0);
        if (S1) Env->DeleteLocalRef(S1);
        if (S2) Env->DeleteLocalRef(S2);

        int32 State = FCString::Atoi(*StateStr);
        if (State == 2 && !KeyStr.IsEmpty()) {
            bWebLoginPending = false;
            jmethodID ResetMethod = Env->GetStaticMethodID(Class, "reset", "()V");
            if (ResetMethod) Env->CallStaticVoidMethod(Class, ResetMethod);
            SetApiKey(KeyStr);
            OnResult(true, KeyStr);
        } else if (State == 3 || State == 4) {
            bWebLoginPending = false;
            jmethodID ResetMethod = Env->GetStaticMethodID(Class, "reset", "()V");
            if (ResetMethod) Env->CallStaticVoidMethod(Class, ResetMethod);
            StatusMessage = MsgStr;
            OnResult(false, MsgStr);
        }
    }
    if (Env->ExceptionCheck()) Env->ExceptionClear();
    if (Array) Env->DeleteLocalRef(Array);
    Env->DeleteLocalRef(Class);
#endif
}
