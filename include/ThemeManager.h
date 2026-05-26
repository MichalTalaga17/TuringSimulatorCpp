#pragma once

/**
 * @file ThemeManager.h
 * @brief Zarządzanie jasnym i ciemnym motywem aplikacji Qt.
 */

#include <QObject>
#include <QString>

/**
 * @brief Zarządza globalnym motywem aplikacji.
 *
 * Klasa wczytuje preferencję użytkownika z QSettings, stosuje paletę i arkusz
 * stylów do QApplication oraz emituje sygnał po zmianie motywu.
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Dostępne warianty kolorystyczne aplikacji.
     */
    enum class Theme
    {
        /// Jasny motyw interfejsu.
        Light,
        /// Ciemny motyw interfejsu.
        Dark
    };

    /**
     * @brief Tworzy menedżera i stosuje zapamiętaną preferencję motywu.
     * @param parent Opcjonalny rodzic obiektu Qt.
     */
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() = default;

    /**
     * @brief Zwraca aktualnie aktywny motyw.
     * @return Bieżący wariant Theme.
     */
    Theme currentTheme() const { return m_currentTheme; }

    /**
     * @brief Ustawia i aplikuje wskazany motyw.
     * @param theme Motyw, który ma zostać użyty globalnie.
     */
    void setTheme(Theme theme);

    /**
     * @brief Przełącza aplikację między jasnym i ciemnym motywem.
     */
    void toggleTheme() { setTheme(m_currentTheme == Theme::Light ? Theme::Dark : Theme::Light); }

    /**
     * @brief Pobiera arkusz stylów dla wskazanego motywu.
     * @param theme Motyw, dla którego ma zostać zwrócony arkusz stylów.
     * @return Arkusz stylów Qt zawierający style aplikacji.
     */
    static QString getStylesheet(Theme theme);

    /**
     * @brief Buduje paletę Qt dla wskazanego motywu.
     * @param theme Motyw, dla którego ma zostać zwrócona paleta.
     * @return Paleta kolorów używana przez aplikację.
     */
    static QPalette getPalette(Theme theme);

    /**
     * @brief Aplikuje motyw bez sprawdzania, czy jest już aktywny.
     *
     * Używane podczas inicjalizacji, gdy aplikacja musi otrzymać arkusz stylów
     * nawet wtedy, gdy zapisany motyw to domyślny wariant jasny.
     *
     * @param theme Motyw do natychmiastowego zastosowania.
     */
    void applyTheme(Theme theme);

private:
    /**
     * @brief Wczytuje zapamiętaną preferencję motywu z QSettings.
     */
    void loadThemePreference();

    /**
     * @brief Zapisuje preferencję motywu w QSettings.
     * @param theme Motyw do zapamiętania.
     */
    void saveThemePreference(Theme theme);

    /// Aktualny motyw aplikacji.
    Theme m_currentTheme;
};
