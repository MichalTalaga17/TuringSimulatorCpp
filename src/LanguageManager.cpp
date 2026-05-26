#include "LanguageManager.h"
#include <QSettings>
#include <QCoreApplication>

LanguageManager& LanguageManager::instance()
{
    static LanguageManager instance;
    return instance;
}

LanguageManager::LanguageManager()
{
    loadDictionaries();

    QSettings settings("TuringSimulator", "LanguageSettings");
    QString langStr = settings.value("language", "pl").toString();
    m_currentLanguage = (langStr == "en") ? Language::English : Language::Polish;
}

void LanguageManager::loadDictionaries()
{
    // === Słownik Polski ===
    m_plDict["main_window_title"] = "Symulator Maszyny Turinga";
    m_plDict["menu_file"] = "Plik";
    m_plDict["menu_file_new"] = "Nowy...";
    m_plDict["menu_file_example"] = "Przykładowa maszyna";
    m_plDict["menu_file_open"] = "Wczytaj z pliku .json";
    m_plDict["menu_file_save"] = "Zapisz";
    m_plDict["menu_file_save_as"] = "Zapisz jako...";
    m_plDict["menu_file_open_db"] = "Wczytaj z bazy danych...";
    m_plDict["menu_file_save_db"] = "Zapisz w bazie danych...";
    m_plDict["menu_file_close_tab"] = "Zamknij kartę";
    m_plDict["menu_file_quit"] = "Zakończ";

    m_plDict["menu_machine"] = "Maszyna";
    m_plDict["menu_machine_validate"] = "Waliduj i przebuduj";
    m_plDict["menu_machine_reset_example"] = "Przywróć przykład startowy";
    m_plDict["menu_machine_generate_ai"] = "Generuj z Gemini AI...";
    m_plDict["menu_machine_gemini_settings"] = "Ustawienia Gemini API...";

    m_plDict["menu_simulation"] = "Symulacja";
    m_plDict["menu_simulation_step"] = "Krok";
    m_plDict["menu_simulation_run_pause"] = "Uruchom / pauza";
    m_plDict["menu_simulation_reset"] = "Reset";

    m_plDict["menu_view"] = "Widok";
    m_plDict["menu_view_prev_tab"] = "Poprzednia karta";
    m_plDict["menu_view_next_tab"] = "Następna karta";
    m_plDict["menu_view_dark_mode"] = "Tryb ciemny";

    m_plDict["menu_help"] = "Pomoc";
    m_plDict["menu_help_about"] = "O programie";
    m_plDict["menu_help_authors"] = "O autorach";

    m_plDict["msg_restart_required"] = "Wymagany restart";
    m_plDict["msg_restart_desc"] = "Aby zmienić język, musisz zrestartować aplikację. Uruchomić ponownie teraz?";

    m_plDict["new_machine_tooltip"] = "Nowa maszyna";

    m_plDict["gemini_API_settings_title"] = "ustawienia Gemini API";
    m_plDict["gemini_API_keys"] = "klucze Gemini API";
    m_plDict["API_tiers_info"] = "Zapisz osobne klucze dla Free tier i Pro tier.";
    m_plDict["Pro_API_key"] = "klucz API pro";
    m_plDict["Free_API_key"] = "darmowy klucz API";

    m_plDict["Gemini_generates"] = "Gemini generuje maszynę...";
    m_plDict["Gemini_generates_fallback"] = "Gemini generuje maszynę przez model fallback...";

    m_plDict["turn_on_thinking"] = "Włącz myślenie modelu";
    m_plDict["generate_with_gemini"] = "Generuj maszynę z Gemini";
    m_plDict["generation_info"] = "Opisz zadanie, wybierz tier API i ustaw tryb myślenia.";
    m_plDict["machine_description"] = "Opis maszyny";
    m_plDict["example_prompt"] = "np. Odejmowanie 1 od liczby zapisanej binarnie na taśmie.";
    m_plDict["Tier:"] = "Tier:";
    m_plDict["Thinking:"] = "Myślenie:";
    m_plDict["generate"] = "generuj";

    m_plDict["API_key_lacks"] = "Brakuje klucza API dla %1. Uzupełnij go w ustawieniach Gemini.";
    m_plDict["generation_in_progress"] = "Generowanie maszyny jest już w toku.";
    m_plDict["AI_error"] = "Błąd AI";
    m_plDict["machine_ready_msg"] = "Maszyna została wygenerowana i wczytana do edytora.";
    m_plDict["generation_canceled"] = "Anulowano generację";

    m_plDict["Run"] = "Uruchom";
    m_plDict["Edit"] = "Edytuj";
    m_plDict["Rebuilt"] = "Przebudowana";
    m_plDict["Accepted"] = "Zaakceptowana";
    m_plDict["Halted"] = "Zatrzymana";
    m_plDict["Running"] = "Uruchomiona";
    m_plDict["Paused"] = "Pauza";
    m_plDict["Reset"] = "Resetuj";
    m_plDict["Ready"] = "Gotowa";

    
    m_plDict["save"] = "Zapisz";
    m_plDict["cancel"] = "Anuluj";

    // === English Dictionary ===
    m_enDict["main_window_title"] = "Turing Machine Simulator";
    m_enDict["menu_file"] = "File";
    m_enDict["menu_file_new"] = "New machine";
    m_enDict["menu_file_example"] = "Open a machine demo";
    m_enDict["menu_file_open"] = "Load from .json file";
    m_enDict["menu_file_save"] = "Save";
    m_enDict["menu_file_save_as"] = "Save As...";
    m_enDict["menu_file_open_db"] = "Load from Database...";
    m_enDict["menu_file_save_db"] = "Save to Database...";
    m_enDict["menu_file_close_tab"] = "Close Tab";
    m_enDict["menu_file_quit"] = "Quit";

    m_enDict["menu_machine"] = "Machine";
    m_enDict["menu_machine_validate"] = "Validate and Rebuild";
    m_enDict["menu_machine_reset_example"] = "Reset to Example";
    m_enDict["menu_machine_generate_ai"] = "Generate with Gemini AI...";
    m_enDict["menu_machine_gemini_settings"] = "Gemini API Settings...";

    m_enDict["menu_simulation"] = "Simulation";
    m_enDict["menu_simulation_step"] = "Step";
    m_enDict["menu_simulation_run_pause"] = "Run / Pause";
    m_enDict["menu_simulation_reset"] = "Reset";

    m_enDict["menu_view"] = "View";
    m_enDict["menu_view_prev_tab"] = "Previous Tab";
    m_enDict["menu_view_next_tab"] = "Next Tab";
    m_enDict["menu_view_dark_mode"] = "Dark Mode";

    m_enDict["menu_help"] = "Help";
    m_enDict["menu_help_about"] = "About";
    m_enDict["menu_help_authors"] = "About Authors";

    m_enDict["msg_restart_required"] = "Restart Required";
    m_enDict["msg_restart_desc"] = "You must restart the application to change language. Restart now?";

    m_enDict["new_machine_tooltip"] = "New Machine";


    m_enDict["gemini_API_settings_title"] = "Gemini API settings";
    m_enDict["gemini_API_keys"] = "Gemini API keys";
    m_enDict["API_tiers_info"] = "Save distinct keys for Free and Pro tier API";
    m_enDict["Pro_API_key"] = "Pro tier API key";
    m_enDict["Free_API_key"] = "Free tier API key";

    m_enDict["Gemini_generates"] = "Gemini generates a machine...";
    m_enDict["Gemini_generates_fallback"] = "Gemini generates a machine with a fallback model...";

    m_enDict["turn_on_thinking"] = "turn on model's thinking";
    m_enDict["generate_with_gemini"] = "Generate a machine with Gemini";
    m_enDict["generation_info"] = "Describe a task, set an API tier and thinking mode.";
    m_enDict["machine_description"] = "machine description";
    m_enDict["example_prompt"] = "fe. Substraction of 1 from a binary number.";
    m_enDict["Tier:"] = "Tier:";
    m_enDict["Thinking:"] = "Thinking:";
    m_enDict["generate"] = "generate";

    m_enDict["API_key_lacks"] = "API key not found, provide it in Gemini settings";
    m_enDict["generation_in_progress"] = "Machine generation already in progress";
    m_enDict["AI_error"] = "AI error";
    m_enDict["machine_ready_msg"] = "Your machine was generated and loaded into the editor";
    m_enDict["generation_canceled"] = "generation canceled";

    m_enDict["Run"] = "Run";
    m_enDict["Edit"] = "Edit";
    m_enDict["Rebuilt"] = "Rebulit";
    m_enDict["Accepted"] = "Accepted";
    m_enDict["Halted"] = "Halted";
    m_enDict["Running"] = "Running";
    m_enDict["Paused"] = "Paused";
    m_enDict["Reset"] = "Reset";
    m_enDict["Ready"] = "Ready";

    m_enDict["save"] = "Save";
    m_enDict["cancel"] = "Cancel";
}

QString LanguageManager::getString(const QString& key) const
{
    const auto& dict = (m_currentLanguage == Language::English) ? m_enDict : m_plDict;
    if (dict.contains(key)) {
        return dict[key];
    }
    return key; // Zwróć klucz jako fallback
}

void LanguageManager::setLanguage(Language lang)
{
    if (m_currentLanguage != lang) {
        m_currentLanguage = lang;
        QSettings settings("TuringSimulator", "LanguageSettings");
        settings.setValue("language", lang == Language::English ? "en" : "pl");

        emit languageChanged();
    }
}

LanguageManager::Language LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}
