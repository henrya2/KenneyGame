#pragma once

#include "KenneyCommons.generated.h"

UENUM(BlueprintType)
enum class EPaperHeroCharacterMoveDirection : uint8
{
	Right,
	Left,
	KeepDir,
};