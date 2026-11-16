#pragma once
#include "CoreMinimal.h"
#include "Misc/Attribute.h"

namespace CrankLocale {
    enum class ELanguage : uint8 {
        Chinese,
        English
    };

    inline ELanguage& ActiveLanguage() {
        static ELanguage Current = ELanguage::Chinese;
        return Current;
    }

    inline bool IsEnglish() {
        return ActiveLanguage() == ELanguage::English;
    }

    inline void SetEnglish(bool bEnable) {
        ActiveLanguage() = bEnable ? ELanguage::English : ELanguage::Chinese;
    }

    inline const TCHAR* Tr(const TCHAR* Zh, const TCHAR* En) {
        return IsEnglish() ? En : Zh;
    }

    inline TAttribute<FText> Text(const TCHAR* Zh, const TCHAR* En) {
        return TAttribute<FText>::CreateLambda([Chinese=FText::FromString(Zh),English=FText::FromString(En)] {
            return IsEnglish() ? English : Chinese;
        });
    }
}
