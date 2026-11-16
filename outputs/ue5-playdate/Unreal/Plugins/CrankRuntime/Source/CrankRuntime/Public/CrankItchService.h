#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

struct CRANKRUNTIME_API FItchUploadInfo {
    int64 Id = 0;
    FString Filename;
    FString DisplayName;
    int64 SizeBytes = 0;
    bool bIsPlaydate = false;
};

struct CRANKRUNTIME_API FItchGameItem {
    int64 GameId = 0;
    int64 DownloadKeyId = 0;
    FString Title;
    FString Url;
    FString CoverUrl;
    FString Author;
    TArray<FItchUploadInfo> Uploads;
};

DECLARE_DELEGATE_TwoParams(FOnItchVerifyCompleted, bool /*bSuccess*/, const FString& /*UsernameOrError*/);
DECLARE_DELEGATE_TwoParams(FOnItchSyncCompleted, bool /*bSuccess*/, const FString& /*Message*/);
DECLARE_DELEGATE_TwoParams(FOnItchDownloadResolved, bool /*bSuccess*/, const FString& /*DownloadUrlOrError*/);

class CRANKRUNTIME_API FCrankItchService : public TSharedFromThis<FCrankItchService> {
public:
    FCrankItchService();
    ~FCrankItchService();

    void SetApiKey(const FString& InApiKey);
    const FString& GetApiKey() const { return ApiKey; }
    bool HasApiKey() const { return !ApiKey.IsEmpty(); }
    const FString& GetUsername() const { return Username; }
    bool IsAuthenticated() const { return bAuthenticated; }
    bool IsBusy() const { return bBusy; }
    const FString& GetStatusMessage() const { return StatusMessage; }
    const TArray<FItchGameItem>& GetOwnedGames() const { return OwnedGames; }

    void VerifyApiKey(const FString& KeyToTest, FOnItchVerifyCompleted OnComplete = FOnItchVerifyCompleted());
    void SyncOwnedLibrary(FOnItchSyncCompleted OnComplete = FOnItchSyncCompleted());
    void ResolveDownloadUrl(int64 DownloadKeyId, int64 UploadId, FOnItchDownloadResolved OnComplete = FOnItchDownloadResolved());

    void ClearAccount();

    void BeginWebLogin();
    void CloseWebLogin();
    bool IsWebLoginActive() const;
    void PollWebLogin(TFunction<void(bool bSuccess, const FString& KeyOrError)> OnResult);

private:
    bool bWebLoginPending = false;
    FString ApiKey;
    FString Username;
    bool bAuthenticated = false;
    bool bBusy = false;
    FString StatusMessage;
    TArray<FItchGameItem> OwnedGames;
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;
};
