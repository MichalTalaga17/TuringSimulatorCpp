#pragma once

/**
 * @file TransitionTableRoles.h
 * @brief Niestandardowe role danych używane przez tabelę przejść.
 */

#include <Qt>

/**
 * @brief Przestrzeń nazw dla pomocniczych elementów UI.
 */
namespace ui
{
/**
 * @brief Rola Qt oznaczająca komórkę aktualnie wykonywanego przejścia.
 */
constexpr int ActiveTransitionRole = Qt::UserRole + 1;
}
