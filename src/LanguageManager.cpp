#include "LanguageManager.h"

#include <QSettings>

namespace
{
struct Translation
{
    const char *key;
    const char *pl;
    const char *en;
};

LanguageManager::Language currentLanguageValue = LanguageManager::Language::Polish;

const Translation translations[] = {
    {"app_title", "Symulator maszyny Turinga", "Turing Machine Simulator"},
    {"menu_file", "Plik", "File"},
    {"menu_machine", "Maszyna", "Machine"},
    {"menu_simulation", "Symulacja", "Simulation"},
    {"menu_view", "Widok", "View"},
    {"menu_language", "Język", "Language"},
    {"menu_help", "Pomoc", "Help"},
    {"language_polish", "Polski", "Polish"},
    {"language_english", "Angielski", "English"},
    {"action_new_blank", "Zacznij od zera", "Start from scratch"},
    {"action_new_example", "Zacznij od przykładu", "Start from example"},
    {"action_open_json", "Wczytaj maszynę z pliku .json", "Load machine from .json file"},
    {"action_save", "Zapisz", "Save"},
    {"action_save_as", "Zapisz jako...", "Save as..."},
    {"action_open_db", "Wczytaj z bazy danych...", "Load from database..."},
    {"action_save_db", "Zapisz w bazie danych...", "Save to database..."},
    {"action_close_tab", "Zamknij kartę", "Close tab"},
    {"action_quit", "Zakończ", "Quit"},
    {"action_validate", "Waliduj i przebuduj", "Validate and rebuild"},
    {"action_reset_example", "Przywróć przykład startowy", "Restore starter example"},
    {"action_generate_ai", "Generuj z Gemini AI...", "Generate with Gemini AI..."},
    {"action_gemini_settings", "Ustawienia Gemini API...", "Gemini API settings..."},
    {"action_step", "Krok", "Step"},
    {"action_run_pause", "Uruchom / pauza", "Run / pause"},
    {"action_reset", "Resetuj", "Reset"},
    {"action_previous_tab", "Poprzednia karta", "Previous tab"},
    {"action_next_tab", "Następna karta", "Next tab"},
    {"action_dark_mode", "Tryb ciemny", "Dark mode"},
    {"action_light_mode", "Tryb jasny", "Light mode"},
    {"action_about_app", "O programie", "About"},
    {"action_about_authors", "O autorach", "Authors"},
    {"tab_start", "Start", "Start"},
    {"new_tab_tooltip", "Nowa maszyna", "New machine"},
    {"close_tab_tooltip", "Zamknij kartę", "Close tab"},
    {"untitled_machine", "Maszyna %1", "Machine %1"},
    {"start_saved_machines", "Zapisane maszyny", "Saved machines"},
    {"start_saved_note", "Maszyny zapisane w lokalnej bazie danych.", "Machines saved in the local database."},
    {"start_empty_saved", "Brak zapisanych maszyn. Po zapisie do bazy pojawią się w tym miejscu.", "No saved machines. Machines saved to the database will appear here."},
    {"start_new_project", "Nowy projekt", "New project"},
    {"start_project_note", "Wybierz gotowy punkt startowy.", "Choose a ready starting point."},
    {"start_blank_project", "Pusty projekt", "Blank project"},
    {"start_blank_tooltip", "Czysta konfiguracja maszyny do samodzielnego uzupełnienia.", "Clean machine configuration for manual editing."},
    {"start_example", "Przykład", "Example"},
    {"start_example_tooltip", "Gotowa maszyna startowa do szybkiego testu symulacji.", "Ready starter machine for a quick simulation test."},
    {"start_ai", "Start z AI", "Start with AI"},
    {"start_ai_tooltip", "Opisz maszynę, a Gemini AI wypełni edytor.", "Describe a machine and Gemini AI will fill the editor."},
    {"dialog_open_machine", "Wczytaj maszynę", "Load machine"},
    {"file_filter_json", "Pliki JSON (*.json)", "JSON files (*.json)"},
    {"about_text", "Symulator maszyny Turinga\n\nEdytuj definicję maszyny, otwieraj wiele plików w osobnych kartach i uruchamiaj symulację bez przełączania okien.", "Turing Machine Simulator\n\nEdit a machine definition, open multiple files in separate tabs, and run simulations without switching windows."},
    {"authors_text", "Program został stworzony przez studentów Politechniki Śląskiej z wydziału matematyki stosowanej\n\nRadosław Kadela, Michał Talaga, Jonatan Turski", "Created by students of the Silesian University of Technology, Faculty of Applied Mathematics\n\nRadosław Kadela, Michał Talaga, Jonatan Turski"},
    {"database_title", "Baza danych", "Database"},
    {"database_empty_text", "Baza danych jest pusta. Zapisz najpierw jakąś maszynę.", "The database is empty. Save a machine first."},
    {"database_load_title", "Wczytaj z bazy", "Load from database"},
    {"database_choose_machine", "Wybierz maszynę:", "Select machine:"},

    {"group_logic_editor", "Edytor logiki", "Logic editor"},
    {"label_states", "Stany:", "States:"},
    {"label_alphabet", "Alfabet:", "Alphabet:"},
    {"label_blank", "Pusty:", "Blank:"},
    {"label_start", "Start:", "Start:"},
    {"ai_button_tooltip", "Wygeneruj maszynę przez Gemini API.", "Generate a machine through Gemini API."},
    {"group_machine_state", "Stan maszyny", "Machine state"},
    {"metric_step", "KROK", "STEP"},
    {"metric_state", "STAN", "STATE"},
    {"metric_mode", "TRYB", "MODE"},
    {"button_step", "Krok", "Step"},
    {"button_run_start", "Start", "Start"},
    {"button_stop", "Zatrzymaj", "Stop"},
    {"button_reset", "Resetuj", "Reset"},
    {"label_speed", "Szybkość:", "Speed:"},
    {"group_input_tape", "Taśma wejściowa", "Input tape"},
    {"label_tape", "Taśma:", "Tape:"},
    {"tape_placeholder", "np. 1001", "e.g. 1001"},
    {"tape_warning", "Taśma zawiera symbole spoza alfabetu. Wykonanie zatrzyma maszynę.", "The tape contains symbols outside the alphabet. Execution will halt the machine."},
    {"group_tape_preview", "Podgląd taśmy", "Tape preview"},
    {"status_ready", "Gotowe", "Ready"},
    {"status_rebuilt", "Przebudowano", "Rebuilt"},
    {"status_editing", "Edycja", "Editing"},
    {"status_built", "Zbudowano", "Built"},
    {"status_halted", "Zatrzymano", "Halted"},
    {"status_running", "Działa", "Running"},
    {"status_paused", "Pauza", "Paused"},
    {"status_reset", "Zresetowano", "Reset"},
    {"status_saved_file", "Zapisano maszynę", "Machine saved"},
    {"status_loaded_file", "Wczytano z pliku", "Loaded from file"},
    {"status_saved_db", "Zapisano w bazie", "Saved in database"},
    {"status_loaded_db", "Wczytano z bazy: %1", "Loaded from database: %1"},
    {"status_ai_generating", "AI...", "AI..."},
    {"status_ai_error", "Błąd AI", "AI error"},
    {"status_ai_generated", "Wygenerowano AI", "AI generated"},
    {"status_ai_canceled", "Anulowano AI", "AI canceled"},
    {"validation_error_title", "Błąd walidacji", "Validation error"},
    {"validation_states", "Ustaw liczbę stanów.", "Set the number of states."},
    {"validation_initial", "Wybierz stan początkowy.", "Select the initial state."},
    {"validation_transition", "Zdefiniuj co najmniej jedno przejście.", "Define at least one transition."},
    {"success_title", "Sukces", "Success"},
    {"build_success_text", "Maszyna została poprawnie zbudowana. Możesz ją uruchomić w panelu powyżej.", "The machine was built successfully. You can run it in the panel above."},
    {"error_title", "Błąd", "Error"},
    {"build_first", "Najpierw zbuduj maszynę.", "Build the machine first."},
    {"save_machine_title", "Zapisz maszynę", "Save machine"},
    {"save_failed", "Nie udało się zapisać pliku.", "Could not save the file."},
    {"save_file_success", "Zapisano pomyślnie do pliku.", "Saved successfully to file."},
    {"save_db_title", "Zapisz do bazy", "Save to database"},
    {"save_db_prompt", "Podaj nazwę dla tej maszyny:", "Enter a name for this machine:"},
    {"save_db_failed", "Wystąpił problem z zapisem do bazy danych.", "There was a problem saving to the database."},
    {"save_db_success", "Zapisano maszynę w bazie danych!", "Machine saved in the database."},
    {"move_left", "L", "L"},
    {"move_right", "P", "R"},
    {"move_stay", "Bez ruchu", "Stay"},
    {"move_stop", "Stop", "Stop"},

    {"gemini_title", "Gemini AI", "Gemini AI"},
    {"gemini_api_title", "Gemini API", "Gemini API"},
    {"gemini_api_settings_title", "Ustawienia Gemini API", "Gemini API settings"},
    {"gemini_api_keys_title", "Klucze Gemini API", "Gemini API keys"},
    {"gemini_api_keys_subtitle", "Zapisz osobne klucze dla wariantu Free i wariantu Pro.", "Save separate keys for the Free and Pro variants."},
    {"gemini_free_key_placeholder", "Klucz API wariantu Free", "Free variant API key"},
    {"gemini_pro_key_placeholder", "Klucz API wariantu Pro", "Pro variant API key"},
    {"tier_free", "Wariant Free", "Wariant Free"},
    {"tier_pro", "Wariant Pro", "Wariant Pro"},
    {"button_save", "Zapisz", "Save"},
    {"button_cancel", "Anuluj", "Cancel"},
    {"button_generate", "Generuj", "Generate"},
    {"gemini_generate_title", "Generuj maszynę z Gemini", "Generate machine with Gemini"},
    {"gemini_generate_subtitle", "Opisz zadanie, wybierz wariant API i ustaw tryb myślenia.", "Describe the task, choose an API variant, and set thinking mode."},
    {"gemini_description_label", "Opis maszyny", "Machine description"},
    {"gemini_description_placeholder", "np. Odejmowanie 1 od liczby zapisanej binarnie na taśmie.", "e.g. Subtract 1 from a binary number written on the tape."},
    {"gemini_tier_label", "Wariant:", "Variant:"},
    {"gemini_thinking_label", "Myślenie:", "Thinking:"},
    {"gemini_thinking_checkbox", "Włącz myślenie modelu", "Enable model thinking"},
    {"gemini_progress", "Gemini generuje maszynę...", "Gemini is generating the machine..."},
    {"gemini_progress_fallback", "Gemini generuje maszynę przez model zapasowy...", "Gemini is generating the machine through the fallback model..."},
    {"gemini_busy", "Generowanie maszyny jest już w toku.", "Machine generation is already in progress."},
    {"gemini_missing_key", "Brakuje klucza API dla %1. Uzupełnij go w ustawieniach Gemini.", "Missing API key for %1. Add it in Gemini settings."},
    {"gemini_generated_success", "Maszyna została wygenerowana i wczytana do edytora.", "The machine was generated and loaded into the editor."},
    {"gemini_invalid_json", "Gemini nie zwrócił poprawnego JSON-a maszyny.", "Gemini did not return valid machine JSON."},
    {"gemini_invalid_blank", "Gemini zwrócił niepoprawny symbol pusty. Użyj jednego znaku ASCII bez spacji i przecinka.", "Gemini returned an invalid blank symbol. Use one ASCII character without spaces or commas."},
    {"gemini_too_many_states", "Gemini zwrócił za dużo stanów. Maksimum w edytorze to %1.", "Gemini returned too many states. The editor maximum is %1."},
    {"gemini_invalid_alphabet", "Gemini zwrócił alfabet z nieobsługiwanym symbolem.", "Gemini returned an alphabet with an unsupported symbol."},
    {"gemini_unsupported_move", "Gemini zwrócił nieobsługiwany ruch głowicy: %1.", "Gemini returned an unsupported head move: %1."},
    {"gemini_invalid_state", "Gemini zwrócił niepoprawny stan: %1. Stany muszą mieć postać q0, q1, ...", "Gemini returned an invalid state: %1. States must be q0, q1, ..."},
    {"gemini_invalid_target_state", "Gemini zwrócił niepoprawny stan docelowy: %1.", "Gemini returned an invalid target state: %1."},
    {"gemini_invalid_transition_symbol", "Gemini zwrócił przejście z nieobsługiwanym symbolem.", "Gemini returned a transition with an unsupported symbol."},
    {"gemini_state_out_of_range", "Gemini zwrócił stan poza zakresem q0...q%1.", "Gemini returned a state outside the q0...q%1 range."},
    {"gemini_timeout", "Przekroczono czas oczekiwania na odpowiedź Gemini.", "Timed out waiting for Gemini response."},
    {"gemini_invalid_api_response", "Gemini API zwróciło niepoprawną odpowiedź: %1.", "Gemini API returned an invalid response: %1."},
    {"gemini_api_error", "Gemini API zwróciło błąd.", "Gemini API returned an error."},
    {"gemini_http_status", "Gemini API zwróciło status HTTP %1.", "Gemini API returned HTTP status %1."},
    {"gemini_empty_content", "Gemini nie zwrócił treści z definicją maszyny.", "Gemini did not return content with a machine definition."},
    {"gemini_max_tokens", "Gemini ucięło odpowiedź (MAX_TOKENS). Spróbuj wyłączyć myślenie albo wybrać mocniejszy model.", "Gemini truncated the response (MAX_TOKENS). Try disabling thinking or selecting a stronger model."},

    {"json_not_object", "Niepoprawny JSON: dokument nie jest obiektem.", "Invalid JSON: document is not an object."},
    {"json_missing_field", "Brak lub niepoprawne pole: %1.", "Missing or invalid field: %1."},
    {"json_field_one_char", "Pole %1 musi zawierać dokładnie jeden znak.", "Field %1 must contain exactly one character."},
    {"json_num_states", "Pole numStates musi być dodatnią liczbą całkowitą.", "Field numStates must be a positive integer."},
    {"json_alphabet", "Pole alphabet musi zawierać listę symboli.", "Field alphabet must contain a list of symbols."},
    {"json_invalid_alphabet_symbol", "Niepoprawny symbol alfabetu pod indeksem %1.", "Invalid alphabet symbol at index %1."},
    {"json_transitions", "Pole transitions musi zawierać listę instrukcji.", "Field transitions must contain a list of instructions."},
    {"json_transition_not_object", "Instrukcja #%1 nie jest obiektem JSON.", "Instruction #%1 is not a JSON object."},
    {"json_invalid_transition", "Niepoprawna instrukcja #%1: %2", "Invalid instruction #%1: %2"},
    {"json_file_open_failed", "Nie można otworzyć pliku.", "Could not open the file."},
    {"json_parse_error", "Niepoprawny JSON: %1", "Invalid JSON: %1"},
    {"db_invalid_field", "Niepoprawna wartość pola %1 w bazie danych.", "Invalid value of field %1 in the database."},
    {"db_open_failed", "Nie można otworzyć bazy danych.", "Could not open the database."},
    {"db_tables_failed", "Nie można przygotować tabel bazy danych.", "Could not prepare database tables."},
    {"db_machine_read_failed", "Nie udało się odczytać maszyny z bazy danych.", "Could not read the machine from the database."},
    {"db_machine_not_found", "Nie znaleziono maszyny w bazie danych.", "Machine not found in the database."},
    {"db_transitions_read_failed", "Nie udało się odczytać instrukcji z bazy danych.", "Could not read instructions from the database."}
};

const Translation *findTranslation(const QString &key)
{
    for (const Translation &translation : translations)
    {
        if (key == QLatin1String(translation.key))
        {
            return &translation;
        }
    }

    return nullptr;
}
} // namespace

LanguageManager::Language LanguageManager::currentLanguage()
{
    return currentLanguageValue;
}

void LanguageManager::setCurrentLanguage(Language language)
{
    currentLanguageValue = language;
    QSettings settings;
    settings.setValue("ui/language", languageCode(language));
}

void LanguageManager::loadFromSettings()
{
    QSettings settings;
    const QString code = settings.value("ui/language", "pl").toString().trimmed().toLower();
    currentLanguageValue = code == "en" ? Language::English : Language::Polish;
}

QString LanguageManager::text(const char *key)
{
    return text(QString::fromLatin1(key));
}

QString LanguageManager::text(const QString &key)
{
    const Translation *translation = findTranslation(key);
    if (!translation)
    {
        return key;
    }

    return QString::fromUtf8(currentLanguageValue == Language::Polish ? translation->pl : translation->en);
}

QString LanguageManager::languageCode(Language language)
{
    return language == Language::English ? "en" : "pl";
}

QString LanguageManager::moveLeftText()
{
    return text("move_left");
}

QString LanguageManager::moveRightText()
{
    return text("move_right");
}

QString LanguageManager::moveStayText()
{
    return text("move_stay");
}

QString LanguageManager::moveStopText()
{
    return text("move_stop");
}

QString LanguageManager::localizedMoveText(QString moveText)
{
    moveText = moveText.trimmed();
    const QString upperMove = moveText.toUpper();

    if (upperMove == "L")
    {
        return moveLeftText();
    }
    if (upperMove == "R" || upperMove == "P" || upperMove == "RIGHT")
    {
        return moveRightText();
    }
    if (upperMove == "_" || upperMove == "S" || upperMove == "STAY" || moveText == "Bez ruchu")
    {
        return moveStayText();
    }
    if (upperMove == "STOP" || upperMove == "HALT" || upperMove == "HALTED")
    {
        return moveStopText();
    }

    return moveText;
}
