#include "CrankOverlay.h"
#include "CrankTheme.h"
#include "CrankLocale.h"
#include "CrankTranslation.h"
#include "CrankItchService.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"

TSharedRef<SWidget> SCrankOverlay::Action(const TAttribute<FText>& Label,TFunction<FReply()> Callback,bool AllowBusy,ECrankGlyph Icon,TFunction<bool()> Selected,bool ShowIcon,bool Toolbar) {
    return SNew(SCrankAction).Device(Device).Label(Label).Icon(Icon).ShowIcon(ShowIcon).WrapLabel(!Toolbar && Icon!=ECrankGlyph::Crank)
        .Compact_Lambda([this,Toolbar]{return Toolbar && GetCachedGeometry().GetLocalSize().X<740;})
        .Selected_Lambda([Selected=MoveTemp(Selected)]{return Selected && Selected();})
        .OnClicked_Lambda(MoveTemp(Callback))
        .IsEnabled_Lambda([this,AllowBusy]{return AllowBusy || !Device.IsValid() || (!Device->IsImportBusy() && !Device->IsGameSwitching());});
}
static TSharedRef<STextBlock> HudText(const TAttribute<FText>& Text,bool Title=false) {
    return SNew(STextBlock).Text(Text).Font(Title ? CrankTheme::Title() : CrankTheme::Body())
        .ColorAndOpacity(CrankTheme::HudText()).AutoWrapText(true);
}
TSharedRef<SWidget> SCrankOverlay::BuildTranslationSettings() {
    auto Service=[this]{return Device.IsValid()?Device->GetTranslation():nullptr;};
    auto Choice=[this,Service](const TAttribute<FText>& Label,const FString& Provider) {
        return Action(Label,[Service,Provider]{if(auto S=Service())S->SelectProvider(Provider);return FReply::Handled();},true,ECrankGlyph::Settings,
            [Service,Provider]{auto S=Service();return S && S->GetProvider()==Provider;},false);
    };
    TranslationSettingsFocus=Choice(CrankLocale::Text(TEXT("自动选择"),TEXT("Auto Select")),TEXT("auto"));
    const bool Local=FCrankTranslation::LocalSupported();
    return SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[TranslationSettingsFocus.ToSharedRef()]
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
     [SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,6,0)[Choice(CrankLocale::Text(TEXT("微软翻译"),TEXT("Microsoft")),TEXT("microsoft"))]
      +SHorizontalBox::Slot().FillWidth(1)[Choice(CrankLocale::Text(TEXT("腾讯翻译"),TEXT("Tencent")),TEXT("tencent"))]]
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
     [SNew(SBox).Visibility(Local?EVisibility::Visible:EVisibility::Collapsed)[Choice(CrankLocale::Text(TEXT("本地翻译"),TEXT("Local Model")),TEXT("local"))]]
     +SVerticalBox::Slot().AutoHeight()
     [SNew(SBox).MinDesiredHeight(32)
      [SNew(STextBlock).Text_Lambda([Service]{auto S=Service();return FText::FromString(S?S->ProviderMessage(CrankLocale::IsEnglish()):FString());})
       .Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::HudMuted()).AutoWrapText(true)]]
     +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)
     [SNew(SBox).Visibility(Local?EVisibility::Visible:EVisibility::Collapsed)
      [SNew(STextBlock).Text_Lambda([Service]{auto S=Service();return FText::FromString(S?S->ModelMessage(CrankLocale::IsEnglish()):FString());})
       .Font(CrankTheme::Utility()).ColorAndOpacity_Lambda([Service]{auto S=Service();return S && S->HasModelError()?CrankTheme::HudError():CrankTheme::HudMuted();}).AutoWrapText(true)]]
     +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)
     [SNew(SBox).HeightOverride(6).Visibility_Lambda([Service]{auto S=Service();return S && S->IsModelBusy()?EVisibility::Visible:EVisibility::Collapsed;})
      [SNew(SProgressBar).Style(&CrankTheme::Progress()).Percent_Lambda([Service]{auto S=Service();return S?S->ModelProgress():TOptional<float>();}).FillColorAndOpacity(CrankTheme::Accent())]]
     +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
     [SNew(SBox).Visibility_Lambda([Service]{auto S=Service();return FCrankTranslation::LocalSupported() && S && !S->HasModel()?EVisibility::Visible:EVisibility::Collapsed;})
      [SNew(SCrankAction).Device(Device).ShowIcon(false)
       .Label_Lambda([Service]{auto S=Service();return FText::FromString(S?S->ModelDownloadLabel(CrankLocale::IsEnglish()):FString());})
       .OnClicked_Lambda([Service]{if(auto S=Service()){if(S->IsModelBusy())S->CancelModelDownload();else S->DownloadModel();}return FReply::Handled();})]];
}
TSharedRef<SWidget> SCrankOverlay::BuildItchSettings() {
    auto Box=SNew(SVerticalBox);

    Box->AddSlot().AutoHeight().Padding(0,0,0,8)
    [SNew(STextBlock).Text_Lambda([this]{return GetItchStatusText();})
     .Font(CrankTheme::Utility())
     .ColorAndOpacity_Lambda([this]{
         return Device.IsValid() && Device->GetItchService().IsValid() && Device->GetItchService()->IsAuthenticated()
             ? CrankTheme::Accent() : CrankTheme::HudMuted();
     }).AutoWrapText(true)];

    Box->AddSlot().AutoHeight().Padding(0,0,0,10)
    [SNew(STextBlock).Text_Lambda([this]{
        return FText::FromString(Device.IsValid() && Device->GetItchService().IsValid() ? Device->GetItchService()->GetStatusMessage() : FString());
    }).Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::HudText()).AutoWrapText(true)];

    Box->AddSlot().AutoHeight().Padding(0,0,0,8)
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,6,0)
     [Action(CrankLocale::Text(TEXT("网页快捷登录"),TEXT("Web Login")),[this]{return OnOpenItchWebLogin();},false,ECrankGlyph::Menu)]
     +SHorizontalBox::Slot().FillWidth(1)
     [Action(CrankLocale::Text(TEXT("粘贴 API 密钥"),TEXT("Paste API Key")),[this]{return OnPasteItchApiKey();},false,ECrankGlyph::Settings)]];

    Box->AddSlot().AutoHeight().Padding(0,0,0,8)
    [Action(CrankLocale::Text(TEXT("同步已购游戏"),TEXT("Sync Library")),[this]{return OnSyncItchLibrary();},false,ECrankGlyph::Library)];

    Box->AddSlot().AutoHeight()
    [SNew(SBox).Visibility_Lambda([this]{
        return Device.IsValid() && Device->GetItchService().IsValid() && Device->GetItchService()->HasApiKey()
            ? EVisibility::Visible : EVisibility::Collapsed;
    })
     [Action(CrankLocale::Text(TEXT("清除密钥 / 注销"),TEXT("Clear Key / Sign Out")),[this]{return OnClearItchAccount();},false,ECrankGlyph::Reset)]];

    return Box;
}

FText SCrankOverlay::GetItchStatusText() const {
    if (!Device.IsValid() || !Device->GetItchService().IsValid()) {
        return CrankLocale::Text(TEXT("服务未就绪"),TEXT("Service not ready")).Get();
    }
    auto Itch=Device->GetItchService();
    if (Itch->IsAuthenticated()) {
        return FText::FromString(FString::Printf(TEXT("%s: %s"),
            CrankLocale::Tr(TEXT("已连接"),TEXT("Connected as")),
            *Itch->GetUsername()));
    }
    return FText::FromString(CrankLocale::Tr(
        TEXT("未连接。从 itch.io/user/settings/api-keys 复制 API 密钥后，点按“粘贴 API 密钥”即可同步已购的 Playdate 游戏。"),
        TEXT("Not connected. Copy an API key from itch.io/user/settings/api-keys, then tap 'Paste API Key' to sync owned Playdate games.")
    ));
}

FReply SCrankOverlay::OnOpenItchWebLogin() {
    if(!Device.IsValid() || !Device->GetItchService().IsValid()) return FReply::Handled();
    Device->GetItchService()->BeginWebLogin();
    return FReply::Handled();
}

FReply SCrankOverlay::OnPasteItchApiKey() {
    if(!Device.IsValid() || !Device->GetItchService().IsValid()) return FReply::Handled();
    FString Key;
    FPlatformApplicationMisc::ClipboardPaste(Key);
    Key.TrimStartAndEndInline();
    if(Key.IsEmpty()) {
        if(Device.IsValid()) Device->PulseHaptic();
        return FReply::Handled();
    }
    Device->GetItchService()->VerifyApiKey(Key, FOnItchVerifyCompleted::CreateLambda([this](bool bSuccess, const FString& UserOrError) {
        if(bSuccess && Device.IsValid()) {
            Device->SaveExperience();
            Device->PulseHaptic();
            Device->GetItchService()->SyncOwnedLibrary(FOnItchSyncCompleted::CreateLambda([this](bool bSyncOk, const FString& Msg) {
                RefreshGames();
            }));
        }
    }));
    return FReply::Handled();
}

FReply SCrankOverlay::OnSyncItchLibrary() {
    if(!Device.IsValid() || !Device->GetItchService().IsValid()) return FReply::Handled();
    Device->GetItchService()->SyncOwnedLibrary(FOnItchSyncCompleted::CreateLambda([this](bool bSuccess, const FString& Message) {
        RefreshGames();
    }));
    return FReply::Handled();
}

FReply SCrankOverlay::OnClearItchAccount() {
    if(!Device.IsValid() || !Device->GetItchService().IsValid()) return FReply::Handled();
    Device->GetItchService()->ClearAccount();
    Device->SaveExperience();
    RefreshGames();
    return FReply::Handled();
}

TSharedRef<SWidget> SCrankOverlay::BuildSettings() {
    auto Content=SNew(SVerticalBox);
    auto Toggle=[&](const TAttribute<FText>& Title,const TAttribute<FText>& Hint,TFunction<bool()> Value,TFunction<void(bool)> Change) {
        TSharedRef<SCheckBox> Check=SNew(SCheckBox).Style(&CrankTheme::Toggle())
          .IsChecked_Lambda([Value]{return Value() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
          .OnCheckStateChanged_Lambda([this,Change](ECheckBoxState State){Change(State==ECheckBoxState::Checked); if(Device.IsValid()) Device->PulseHaptic();})
          [SNew(SVerticalBox)
           +SVerticalBox::Slot().AutoHeight()
           [SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1)[HudText(Title)]
            +SHorizontalBox::Slot().AutoWidth().Padding(12,0,0,0)
            [SNew(STextBlock).Text_Lambda([Value]{return FText::FromString(Value() ? CrankLocale::Tr(TEXT("开启"),TEXT("ON")) : CrankLocale::Tr(TEXT("关闭"),TEXT("OFF")));})
             .Font(CrankTheme::Utility()).ColorAndOpacity_Lambda([Value]{return Value() ? CrankTheme::Accent() : CrankTheme::HudMuted();})]]
           +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
           [SNew(STextBlock).Text(Hint).Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::HudMuted()).AutoWrapText(true)]];
        if(!SettingsFocus) SettingsFocus=Check;
        Content->AddSlot().AutoHeight().Padding(0,0,0,12)[SNew(SBox).MinDesiredHeight(CrankTheme::Target)[Check]];
    };
    Toggle(CrankLocale::Text(TEXT("英文界面 (English UI)"),TEXT("English UI")),
        CrankLocale::Text(TEXT("将模拟器菜单、HUD 与设置切换为英文"),TEXT("Display menus, HUD and settings in English")),
        [this]{return Device.IsValid() && Device->IsEnglishUI();},
        [this](bool V){if(Device.IsValid()) Device->SetEnglishUI(V);});
    Toggle(CrankLocale::Text(TEXT("沉浸式模式"),TEXT("Immersive Mode")),
        CrankLocale::Text(TEXT("放大屏幕与按键；屏幕上下滑动可转动摇柄"),TEXT("Enlarge display & controls; swipe up/down to crank")),
        [this]{return Device.IsValid() && Device->IsImmersive();},
        [this](bool V){if(Device.IsValid()) Device->SetImmersive(V);});
    Toggle(CrankLocale::Text(TEXT("摇柄收起"),TEXT("Dock Crank")),
        CrankLocale::Text(TEXT("收起时停用摇柄；关闭此项即可展开"),TEXT("Disable crank when docked; toggle to undock")),
        [this]{return Device.IsValid() && Device->IsCrankDocked();},
        [this](bool V){if(Device.IsValid() && V!=Device->IsCrankDocked()) Device->ToggleCrankDocked();});
    Toggle(CrankLocale::Text(TEXT("视角跟随"),TEXT("Motion Follow")),
        CrankLocale::Text(TEXT("在 3D 模式中跟随手机倾斜"),TEXT("Follow device tilt in 3D mode")),
        [this]{return Device.IsValid() && Device->IsMotionFollowEnabled();},
        [this](bool V){if(Device.IsValid()) Device->SetMotionFollow(V);});
    Content->AddSlot().AutoHeight().Padding(0,0,0,18)
    [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Device.IsValid() && Device->IsImmersive() ? CrankLocale::Tr(TEXT("沉浸式模式不移动视角；回到 3D 后继续跟随"),TEXT("Fixed view in immersive mode; resumes in 3D")) : Device.IsValid() && Device->IsMotionFollowEnabled() && !Device->HasMotionSample() ? CrankLocale::Tr(TEXT("等待运动数据；可拖动背景调整视角"),TEXT("Waiting for motion data; drag background to rotate")) : CrankLocale::Tr(TEXT("双指缩放；拖动背景旋转"),TEXT("Pinch to zoom; drag background to rotate")));})
     .Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::HudMuted()).AutoWrapText(true)];
    Content->AddSlot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]{
        const float Percent=Device.IsValid() ? Device->GetMotionSensitivity()*100 : 35.f;
        return FText::FromString(FString::Printf(TEXT("%s  %.0f%%"),CrankLocale::Tr(TEXT("跟随灵敏度"),TEXT("Motion Sensitivity")),Percent));
    })
        .Font(CrankTheme::Body()).ColorAndOpacity(CrankTheme::HudText())];
    Content->AddSlot().AutoHeight().Padding(0,8,0,18)
    [SNew(SBox).MinDesiredHeight(40)
     [SNew(SSlider).Style(&CrankTheme::Slider()).MinValue(.1f).MaxValue(.8f).StepSize(.05f)
      .Value_Lambda([this]{return Device.IsValid() ? Device->GetMotionSensitivity() : .35f;})
      .SliderHandleColor(CrankTheme::Accent()).SliderBarColor(CrankTheme::HudFrame())
      .OnValueChanged_Lambda([this](float V){if(Device.IsValid()) Device->SetMotionSensitivity(V);})]];
    Content->AddSlot().AutoHeight().Padding(0,0,0,20)
    [Action(CrankLocale::Text(TEXT("校准当前握持姿势"),TEXT("Calibrate Holding Pose")),[this]{if(Device.IsValid()) {Device->CalibrateMotion(); Device->PulseHaptic();} return FReply::Handled();},true,ECrankGlyph::Reset)];
    Toggle(CrankLocale::Text(TEXT("操作震动"),TEXT("Haptics")),
        CrankLocale::Text(TEXT("按钮短反馈，摇柄转动带刻度感"),TEXT("Button click feedback and tactile crank ticks")),
        [this]{return Device.IsValid() && Device->IsHapticsEnabled();},
        [this](bool V){if(Device.IsValid()) Device->SetHaptics(V);});
    Content->AddSlot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]{
        const float Percent=Device.IsValid() ? Device->GetHapticStrength()*100 : 60.f;
        return FText::FromString(FString::Printf(TEXT("%s  %.0f%%"),CrankLocale::Tr(TEXT("震动强度"),TEXT("Haptic Strength")),Percent));
    })
        .Font(CrankTheme::Body()).ColorAndOpacity(CrankTheme::HudText())];
    Content->AddSlot().AutoHeight().Padding(0,8,0,20)
    [SNew(SBox).MinDesiredHeight(40)
     [SNew(SSlider).Style(&CrankTheme::Slider()).MinValue(.1f).MaxValue(1.f).StepSize(.1f)
      .Value_Lambda([this]{return Device.IsValid() ? Device->GetHapticStrength() : .6f;})
      .SliderHandleColor(CrankTheme::Accent()).SliderBarColor(CrankTheme::HudFrame())
      .OnValueChanged_Lambda([this](float V){if(Device.IsValid()) Device->SetHapticStrength(V);})]];
    Toggle(CrankLocale::Text(TEXT("减少界面动效"),TEXT("Reduce Motion")),
        CrankLocale::Text(TEXT("切换视图直接完成，保留操作反馈"),TEXT("Instant transitions without camera animations")),
        [this]{return Device.IsValid() && Device->IsReducedMotion();},
        [this](bool V){if(Device.IsValid()) Device->SetReducedMotion(V);});
    Content->AddSlot().AutoHeight().Padding(0,8,0,8)[HudText(CrankLocale::Text(TEXT("画面与性能"),TEXT("Display & Performance")),true)];
    Content->AddSlot().AutoHeight().Padding(0,0,0,8)
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,6,0)[Action(CrankLocale::Text(TEXT("30 帧 · 省电"),TEXT("30 FPS · Eco")),[this]{if(Device.IsValid()) Device->SetFrameRate(30); return FReply::Handled();},true,ECrankGlyph::Settings,[this]{return Device.IsValid() && Device->GetFrameRate()==30;},false)]
     +SHorizontalBox::Slot().FillWidth(1)[Action(CrankLocale::Text(TEXT("60 帧 · 流畅"),TEXT("60 FPS · Smooth")),[this]{if(Device.IsValid()) Device->SetFrameRate(60); return FReply::Handled();},true,ECrankGlyph::Settings,[this]{return Device.IsValid() && Device->GetFrameRate()==60;},false)]];
    Content->AddSlot().AutoHeight().Padding(0,0,0,20)
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,6,0)[Action(CrankLocale::Text(TEXT("标准画面"),TEXT("Standard Quality")),[this]{if(Device.IsValid()) Device->SetResolutionScale(.75f); return FReply::Handled();},true,ECrankGlyph::Settings,[this]{return Device.IsValid() && Device->GetResolutionScale()<.9f;},false)]
     +SHorizontalBox::Slot().FillWidth(1)[Action(CrankLocale::Text(TEXT("高清画面"),TEXT("High Definition")),[this]{if(Device.IsValid()) Device->SetResolutionScale(1); return FReply::Handled();},true,ECrankGlyph::Settings,[this]{return Device.IsValid() && Device->GetResolutionScale()>=.9f;},false)]];
    Content->AddSlot().AutoHeight()
    [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Device.IsValid() ? Device->GetExperienceError() : FString());})
     .Font(CrankTheme::Body()).ColorAndOpacity(CrankTheme::HudError()).AutoWrapText(true)];
    Content->AddSlot().AutoHeight().Padding(0,20,0,8)[HudText(CrankLocale::Text(TEXT("游戏介绍翻译"),TEXT("Game Description Translation")),true)];
    Content->AddSlot().AutoHeight()[BuildTranslationSettings()];
    Content->AddSlot().AutoHeight().Padding(0,20,0,8)[HudText(CrankLocale::Text(TEXT("Itch.io 账号同步"),TEXT("Itch.io Library Sync")),true)];
    Content->AddSlot().AutoHeight()[BuildItchSettings()];
    return Content;
}
void SCrankOverlay::Construct(const FArguments& Args) {
    Device=Args._Device;
    LibraryButton=Action(CrankLocale::Text(TEXT("游戏"),TEXT("Games")),[this]{return ToggleLibrary();},true,ECrankGlyph::Library,[this]{return bLibraryOpen;},true,true);
    SettingsButton=Action(CrankLocale::Text(TEXT("设置"),TEXT("Settings")),[this]{return ToggleSettings();},true,ECrankGlyph::Settings,[this]{return bSettingsOpen;},true,true);
    auto Library=SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[SAssignNew(GameRows,SVerticalBox)];
#if PLATFORM_ANDROID
    Library->AddSlot().AutoHeight().Padding(0,16,0,8)[Action(CrankLocale::Text(TEXT("导入 ZIP 游戏包"),TEXT("Import ZIP Package")),[this]{if(Device.IsValid()) Device->BeginGameImport(false); return FReply::Handled();})];
    Library->AddSlot().AutoHeight()[Action(CrankLocale::Text(TEXT("选择 PDX 文件夹"),TEXT("Select PDX Folder")),[this]{if(Device.IsValid()) Device->BeginGameImport(true); return FReply::Handled();})];
    Library->AddSlot().AutoHeight().Padding(0,16,0,8)[HudText(CrankLocale::Text(TEXT("ZIP 游戏直链"),TEXT("Direct ZIP URL")))];
    Library->AddSlot().AutoHeight()
    [SAssignNew(UrlInput,SEditableTextBox).Style(&CrankTheme::Input()).Font(CrankTheme::Body()).RevertTextOnEscape(false)
     .HintText(FText::FromString(TEXT("https://…/game.zip")))];
    Library->AddSlot().AutoHeight().Padding(0,10,0,0)
    [Action(CrankLocale::Text(TEXT("下载并安装"),TEXT("Download & Install")),[this]{if(Device.IsValid() && UrlInput) Device->BeginUrlGameImport(UrlInput->GetText().ToString()); return FReply::Handled();})];
#else
    Library->AddSlot().AutoHeight().Padding(0,16,0,8)[HudText(CrankLocale::Text(TEXT("PDX 文件夹"),TEXT("PDX Folder")))];
    Library->AddSlot().AutoHeight()
    [SAssignNew(DirectoryInput,SEditableTextBox).Style(&CrankTheme::Input()).Font(CrankTheme::Body()).RevertTextOnEscape(false)
     .HintText(CrankLocale::Text(TEXT("粘贴游戏文件夹路径"),TEXT("Paste the game folder path")))];
    Library->AddSlot().AutoHeight().Padding(0,12,0,0)[Action(CrankLocale::Text(TEXT("打开游戏"),TEXT("Open Game")),[this]{return OpenDirectory();})];
#endif
    Library->AddSlot().AutoHeight().Padding(0,12,0,0)
    [SNew(SBox).HeightOverride(6).Visibility_Lambda([this]{return Device.IsValid() && Device->IsImportBusy() ? EVisibility::Visible : EVisibility::Collapsed;})
     [SNew(SProgressBar).Style(&CrankTheme::Progress()).Percent(TOptional<float>()).FillColorAndOpacity(CrankTheme::Accent())]];
    Library->AddSlot().AutoHeight().Padding(0,10,0,0)
    [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Device.IsValid() ? Device->GetImportMessage() : FString());})
     .Visibility_Lambda([this]{return Device.IsValid() && !Device->GetImportMessage().IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;})
     .Font(CrankTheme::Body()).ColorAndOpacity(CrankTheme::HudMuted()).AutoWrapText(true)];
    Library->AddSlot().AutoHeight().Padding(0,10,0,0)
    [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Device.IsValid() && Device->HasError() ? Device->GetStatusText() : FString());})
     .Visibility_Lambda([this]{return Device.IsValid() && Device->HasError() ? EVisibility::Visible : EVisibility::Collapsed;})
     .Font(CrankTheme::Body()).ColorAndOpacity(CrankTheme::HudError()).AutoWrapText(true)];
    Library->AddSlot().AutoHeight().Padding(0,12,0,0)
    [SNew(SBox).Visibility_Lambda([this]{return Device.IsValid() && Device->IsLauncherDownloadBusy() ? EVisibility::Visible : EVisibility::Collapsed;})
     [Action(CrankLocale::Text(TEXT("取消下载"),TEXT("Cancel Download")),[this]{if(Device.IsValid()) Device->CancelLauncherDownload();return FReply::Handled();},true,ECrankGlyph::Reset)]];
    Library->AddSlot().AutoHeight().Padding(0,12,0,0)
    [SNew(SBox).Visibility_Lambda([this]{return Device.IsValid() && Device->CanRetryImport() ? EVisibility::Visible : EVisibility::Collapsed;})
     [Action(CrankLocale::Text(TEXT("重试下载 / 导入"),TEXT("Retry Download / Import")),[this]{if(Device.IsValid()) Device->RetryGameImport(); return FReply::Handled();},true,ECrankGlyph::Reset)]];
    auto Tools=SNew(SHorizontalBox)
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[LibraryButton.ToSharedRef()]
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[SettingsButton.ToSharedRef()]
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
        [SNew(SCrankAction).Device(Device).Icon(ECrankGlyph::Screen).WrapLabel(false)
         .Label_Lambda([this]{return FText::FromString(Device.IsValid() && Device->IsImmersive() ? CrankLocale::Tr(TEXT("返回 3D"),TEXT("3D")) : CrankLocale::Tr(TEXT("沉浸"),TEXT("Immersive")));})
         .Compact_Lambda([this]{return GetCachedGeometry().GetLocalSize().X<740;})
         .Selected_Lambda([this]{return Device.IsValid() && Device->IsImmersive();})
         .OnClicked_Lambda([this]{if(Device.IsValid()) Device->SetImmersive(!Device->IsImmersive()); FSlateApplication::Get().SetAllUserFocusToGameViewport(); return FReply::Handled();})]
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
        [SNew(SBox).Visibility_Lambda([this]{return Device.IsValid() && Device->IsImmersive() ? EVisibility::Collapsed : EVisibility::Visible;})
         [Action(CrankLocale::Text(TEXT("放大"),TEXT("Zoom")),[this]{if(Device.IsValid()) Device->ToggleEnlarged(); return FReply::Handled();},true,ECrankGlyph::Expand,[this]{return Device.IsValid() && Device->IsEnlarged();},true,true)]]
        +SHorizontalBox::Slot().AutoWidth()
        [SNew(SBox).Visibility_Lambda([this]{return Device.IsValid() && Device->IsImmersive() ? EVisibility::Collapsed : EVisibility::Visible;})
         [Action(CrankLocale::Text(TEXT("复位"),TEXT("Reset")),[this]{if(Device.IsValid()) Device->ResetView(); return FReply::Handled();},true,ECrankGlyph::Reset,nullptr,true,true)]]
        +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
        [SNew(SBox).Visibility_Lambda([this]{return Device.IsValid() && Device->IsImmersive() ? EVisibility::Visible : EVisibility::Collapsed;})
         .IsEnabled_Lambda([this]{return Device.IsValid() && !Device->IsOverlayOpen();})
         [Action(CrankLocale::Text(TEXT("菜单"),TEXT("Menu")),[this]{if(Device.IsValid()) Device->ToggleSystemMenu();return FReply::Handled();},true,ECrankGlyph::Menu,nullptr,true,true)]]
        +SHorizontalBox::Slot().AutoWidth()
        [SNew(SBox).Visibility_Lambda([this]{return Device.IsValid() && Device->IsImmersive() ? EVisibility::Visible : EVisibility::Collapsed;})
         .IsEnabled_Lambda([this]{return Device.IsValid() && !Device->IsOverlayOpen();})
         [Action(CrankLocale::Text(TEXT("锁屏"),TEXT("Lock")),[this]{if(Device.IsValid()) Device->ToggleLock();return FReply::Handled();},true,ECrankGlyph::Lock,nullptr,true,true)]];
    ChildSlot
    [SNew(SSafeZone).Padding(FMargin(CrankTheme::Edge)).Visibility(EVisibility::SelfHitTestInvisible)
     [SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
      +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)
      [SNew(SBox).MaxDesiredWidth(140).Visibility_Lambda([this]{return GetCachedGeometry().GetLocalSize().X<650 ? EVisibility::Collapsed : EVisibility::HitTestInvisible;})
       [SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,4)
        [SNew(STextBlock).Text(FText::FromString(TEXT("Playdate"))).Font(CrankTheme::Label()).ColorAndOpacity(CrankTheme::Ink())]
        +SVerticalBox::Slot().AutoHeight()
        [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Device.IsValid() ? Device->GetGameTitle() : FString());})
         .Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::Ink()).AutoWrapText(true)]]]
      +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)[Tools]
      +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
      [SAssignNew(PanelContainer,SBox).WidthOverride_Lambda([this]{const FVector2D Size=GetCachedGeometry().GetLocalSize();return FMath::Min(380.f,float(Size.X)-2*CrankTheme::Edge);})
       .HeightOverride_Lambda([this]{const FVector2D Size=GetCachedGeometry().GetLocalSize();const float Available=FMath::Max(1.f,float(Size.Y)-2*CrankTheme::Edge);return FMath::Clamp(float(Size.Y)*.7f,FMath::Min(240.f,Available),FMath::Min(620.f,Available));})
       .Visibility_Lambda([this]{return bLibraryOpen || bSettingsOpen ? EVisibility::Visible : (PanelBlend>.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed);})
       [SNew(SBorder).BorderImage(&CrankTheme::Panel()).Padding(22)
        [SNew(SVerticalBox)
         +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)
         [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(bSettingsOpen ? CrankLocale::Tr(TEXT("体验设置"),TEXT("Preferences")) : CrankLocale::Tr(TEXT("游戏库"),TEXT("Game Library")));}).Font(CrankTheme::Title()).ColorAndOpacity(CrankTheme::HudText())]
         +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
         [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(bSettingsOpen ? CrankLocale::Tr(TEXT("让这台掌机适应你的握持方式"),TEXT("Adjust console to your holding style")) : CrankLocale::Tr(TEXT("选择游戏，继续游玩"),TEXT("Select a game to start playing")));})
          .Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::HudMuted()).AutoWrapText(true)]
         +SVerticalBox::Slot().FillHeight(1)
         [SNew(SOverlay)
          +SOverlay::Slot()
          [SAssignNew(GameScroll,SScrollBox).Visibility_Lambda([this]{return bLibraryOpen ? EVisibility::Visible : EVisibility::Collapsed;})+SScrollBox::Slot()[Library]]
          +SOverlay::Slot()
          [SAssignNew(SettingsScroll,SScrollBox).Visibility_Lambda([this]{return bSettingsOpen ? EVisibility::Visible : EVisibility::Collapsed;})+SScrollBox::Slot()[BuildSettings()]]]
         +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)
     [Action(CrankLocale::Text(TEXT("返回游戏"),TEXT("Resume Game")),[this]{return ClosePanels();},true,ECrankGlyph::Reset)]]]]
      +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom)
      [SNew(SBorder).BorderImage(&CrankTheme::Panel()).Padding(FMargin(12,9))
       .Visibility_Lambda([this]{return !bLibraryOpen && !bSettingsOpen && Device.IsValid() && !Device->IsImmersive() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;})
       [SNew(STextBlock).Text(CrankLocale::Text(TEXT("双击显示屏放大 · 拖动背景调整视角"),TEXT("Double-tap to zoom · Drag to pan"))).Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::HudMuted())]]
      +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom)
      .Padding(TAttribute<FMargin>::CreateLambda([this]{return FMargin(0,0,0,Device.IsValid() && Device->IsImmersive() ? 0 : 52);}))
      [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Device.IsValid() ? Device->GetStatusText() : FString());})
       .Visibility_Lambda([this]{return !bLibraryOpen && !bSettingsOpen && Device.IsValid() && (Device->HasError() || !Device->GetLauncherMessage().IsEmpty()) ? EVisibility::Visible : EVisibility::Collapsed;})
       .Font(CrankTheme::Body()).ColorAndOpacity_Lambda([this]{return Device.IsValid() && Device->HasError() ? CrankTheme::Error() : CrankTheme::Ink();}).WrapTextAt(320)]
      +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
      [SNew(SBorder).BorderImage(&CrankTheme::Panel()).Padding(10)
       .Visibility_Lambda([this]{return !bLibraryOpen && !bSettingsOpen && Device.IsValid() && !Device->IsImmersive() ? EVisibility::Visible : EVisibility::Collapsed;})
       [SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()
        [Action(CrankLocale::Text(TEXT("摇柄"),TEXT("Crank")),[this]{bDialExpanded=!bDialExpanded; return FReply::Handled();},true,ECrankGlyph::Crank)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,6)
        [SNew(SCrankDial).Device(Device).Visibility_Lambda([this]{return bDialExpanded ? EVisibility::Visible : EVisibility::Collapsed;})]
        +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,6)
        [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Device.IsValid() && Device->IsCrankDocked() ? CrankLocale::Tr(TEXT("已收起"),TEXT("Docked")) : FString::Printf(TEXT("%.0f°"),Device.IsValid() ? FMath::Fmod(Device->GetCrankAngle()+360000.f,360.f) : 0.f));})
         .Font(CrankTheme::Utility()).ColorAndOpacity(CrankTheme::Accent())]
        +SVerticalBox::Slot().AutoHeight()
        [SNew(SCrankAction).Device(Device).ShowIcon(false).Label_Lambda([this]{return FText::FromString(Device.IsValid() && Device->IsCrankDocked() ? CrankLocale::Tr(TEXT("展开"),TEXT("Undock")) : CrankLocale::Tr(TEXT("收起"),TEXT("Dock")));})
         .OnClicked_Lambda([this]{if(Device.IsValid()) Device->ToggleCrankDocked(); return FReply::Handled();})]]]]];
    RefreshGames();
}
void SCrankOverlay::RefreshGames() {
    GameRows->ClearChildren(); GameButtons.Empty(); if(!Device.IsValid()) return;
    const auto Games=Device->GetGameDirectories();
    if(!Games.Num()) GameRows->AddSlot().AutoHeight()[HudText(CrankLocale::Text(TEXT("还没有游戏。导入 ZIP 或选择一个 PDX 文件夹。"),TEXT("No games yet. Import a ZIP or PDX folder.")))];
    int Index=0;
    for(const FString& Path:Games) {
        const bool Current=Path==Device->GetCurrentGameDirectory() && Device->IsGameRunning(); if(Current) FocusedGame=Index;
        TSharedRef<SWidget> Row=SNew(SCrankAction).Device(Device).Label(FText::FromString(Device->GetTitleForGame(Path)))
            .Subtitle(FText::FromString(Device->GetGameCapability(Path))).Cover(Device->GetGameCover(Path))
            .Selected_Lambda([this,Path]{return Device.IsValid() && Path==Device->GetCurrentGameDirectory() && Device->IsGameRunning();})
            .OnClicked_Lambda([this,Path]{return ChooseGame(Path);})
            .IsEnabled_Lambda([this]{return Device.IsValid() && !Device->IsImportBusy() && !Device->IsGameSwitching();});
        GameButtons.Add(Row); GameRows->AddSlot().AutoHeight().Padding(0,0,0,8)[Row]; Index++;
    }
    if(Device.IsValid() && Device->GetItchService().IsValid()) {
        const auto& ItchGames=Device->GetItchService()->GetOwnedGames();
        if(ItchGames.Num()>0) {
            GameRows->AddSlot().AutoHeight().Padding(0,16,0,8)[HudText(CrankLocale::Text(TEXT("Itch.io 已购游戏"),TEXT("Itch.io Owned Games")),true)];
            for(const auto& Item:ItchGames) {
                TSharedRef<SWidget> ItchRow=SNew(SCrankAction).Device(Device)
                    .Label(FText::FromString(Item.Title))
                    .Subtitle(FText::FromString(Item.Author.IsEmpty() ? TEXT("Itch.io") : Item.Author))
                    .OnClicked_Lambda([this,Item]{
                        if(Device.IsValid() && Device->GetItchService().IsValid()) {
                            Device->GetItchService()->ResolveDownloadUrl(Item.DownloadKeyId,0,FOnItchDownloadResolved::CreateLambda([this](bool bOk,const FString& Url){
                                if(bOk && Device.IsValid()) {
#if PLATFORM_ANDROID
                                    Device->BeginUrlGameImport(Url);
#endif
                                }
                            }));
                        }
                        return FReply::Handled();
                    });
                GameButtons.Add(ItchRow);
                GameRows->AddSlot().AutoHeight().Padding(0,0,0,8)[ItchRow];
            }
        }
    }
    FocusedGame=FMath::Clamp(FocusedGame,0,FMath::Max(0,GameButtons.Num()-1));
}
FReply SCrankOverlay::ToggleLibrary() {
    if(bLibraryOpen) return ClosePanels();
    bSettingsOpen=false; bLibraryOpen=true;
    if(Device.IsValid()) Device->SetOverlayOpen(true);
    RefreshGames();
    if(GameButtons.Num()) FSlateApplication::Get().SetKeyboardFocus(GameButtons[FocusedGame],EFocusCause::SetDirectly);
    else if(DirectoryInput) FSlateApplication::Get().SetKeyboardFocus(DirectoryInput,EFocusCause::SetDirectly);
    return FReply::Handled();
}
FReply SCrankOverlay::ToggleSettings() {
    if(bSettingsOpen) return ClosePanels();
    bLibraryOpen=false; bSettingsOpen=true; if(Device.IsValid()) Device->SetOverlayOpen(true);
    if(SettingsFocus) FSlateApplication::Get().SetKeyboardFocus(SettingsFocus,EFocusCause::SetDirectly);
    return FReply::Handled();
}
FReply SCrankOverlay::ClosePanels() {
    const bool Settings=bSettingsOpen;
    bLibraryOpen=false; bSettingsOpen=false;
    if(Device.IsValid()) Device->SetOverlayOpen(false);
    FSlateApplication::Get().SetKeyboardFocus(Settings ? SettingsButton : LibraryButton,EFocusCause::SetDirectly);
    return FReply::Handled();
}
void SCrankOverlay::ShowLibrary() { if(!bLibraryOpen) ToggleLibrary(); }
void SCrankOverlay::ShowSettings(bool TranslationSettings) {
    if(!bSettingsOpen)ToggleSettings();
    if(TranslationSettings && SettingsScroll && Device.IsValid()) {
        const auto Target=TranslationSettingsFocus;
        if(Target) {
            FSlateApplication::Get().SetKeyboardFocus(Target,EFocusCause::SetDirectly);
            SettingsScroll->ScrollToEnd();
            // Auto-wrapped labels need a visible layout pass before their final height is known.
            RegisterActiveTimer(.1f,FWidgetActiveTimerDelegate::CreateLambda([this,Weak=TWeakPtr<SWidget>(Target)](double,float) {
                if(bSettingsOpen && SettingsScroll && Weak.IsValid())
                    SettingsScroll->ScrollToEnd();
                return EActiveTimerReturnType::Stop;
            }));
        }
    }
}
FReply SCrankOverlay::ChooseGame(const FString& Directory) {
    if(Device.IsValid() && Device->PlayGame(Directory)) ClosePanels();
    return FReply::Handled();
}
FReply SCrankOverlay::OpenDirectory() {
    return DirectoryInput ? ChooseGame(DirectoryInput->GetText().ToString().TrimStartAndEnd().TrimQuotes()) : FReply::Handled();
}
FReply SCrankOverlay::OnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) {
    if((bLibraryOpen || bSettingsOpen) && Event.GetKey()==EKeys::Escape) return ClosePanels();
    if(bLibraryOpen && GameButtons.Num() && (Event.GetKey()==EKeys::Up || Event.GetKey()==EKeys::Down)) {
        FocusedGame=(FocusedGame+(Event.GetKey()==EKeys::Down ? 1 : GameButtons.Num()-1))%GameButtons.Num();
        GameScroll->ScrollDescendantIntoView(GameButtons[FocusedGame],false);
        FSlateApplication::Get().SetKeyboardFocus(GameButtons[FocusedGame],EFocusCause::Navigation); return FReply::Handled();
    }
    return SCompoundWidget::OnKeyDown(Geometry,Event);
}
void SCrankOverlay::Tick(const FGeometry& Geometry,double Time,float DeltaTime) {
    SCompoundWidget::Tick(Geometry,Time,DeltaTime);
    if(Device.IsValid() && Device->GetItchService().IsValid() && Device->GetItchService()->IsWebLoginActive()) {
        Device->GetItchService()->PollWebLogin([this](bool bSuccess, const FString& KeyOrErr) {
            if(bSuccess && Device.IsValid()) {
                Device->SaveExperience();
                Device->PulseHaptic();
                Device->GetItchService()->SyncOwnedLibrary(FOnItchSyncCompleted::CreateLambda([this](bool bSyncOk, const FString& Msg) {
                    RefreshGames();
                }));
            }
        });
    }
    const float Alpha=Device.IsValid() && Device->IsReducedMotion() ? 1.f : CrankView::SmoothAlpha(16,DeltaTime);
    PanelBlend=FMath::Lerp(PanelBlend,bLibraryOpen || bSettingsOpen ? 1.f : 0.f,Alpha);
    PanelContainer->SetRenderOpacity(PanelBlend);
    PanelContainer->SetRenderTransform(FSlateRenderTransform(FVector2D(0,(1-PanelBlend)*16)));
}

