#pragma once

#include <string>

enum class AppLanguage : int {
    RU = 0,
    EN = 1,
    UA = 2,
    COUNT = 3
};

enum class Str : int {
    FOLDERS = 0,
    NEW_FOLDER_HINT,
    ADD_TRACKS,
    SETTINGS_BTN,
    MODE_NORMAL,
    MODE_REPEAT,
    MODE_LOOP,
    MODE_SHUFFLE,
    STOP_ALL,
    COL_TITLE,
    COL_HOTKEY,
    COL_VOLUME,
    COL_STATE,
    COL_DEL,
    BTN_BIND,
    BTN_PLAY,
    BTN_PAUSE,
    BTN_RESUME,
    BTN_STOP,
    BTN_LOAD,
    NOW_PLAYING,
    IDLE,
    VOL_FORMAT,
    BARS_FORMAT,
    TOOLTIP_TRACK,
    TOOLTIP_BARS,
    SETTINGS_TITLE,
    LANG_SELECT,
    CLOSE,
    DEL_FOLDER_CTX,
    HOTKEY_TITLE,
    HOTKEY_DESC,
    HOTKEY_CLEAR,
    HOTKEY_CANCEL,
    HOTKEY_HINT,
    MOVE_FOLDER,
    MOVE_TRACK,
    DEFAULT_FOLDER,
    DEL_FOLDER_TOOLTIP,
    TRAY_MINIMIZE_CHECKBOX,
    TRAY_MINIMIZED_MSG,
    BTN_MINIMIZE_TRAY,
    EQ_BTN,
    EQ_TITLE,
    EQ_ENABLE,
    EQ_LOW,
    EQ_MID,
    EQ_HIGH,
    EQ_RESET,
    COUNT
};

class I18n {
public:
    static AppLanguage& currentLang() {
        static AppLanguage lang = AppLanguage::RU;
        return lang;
    }

    static void setLanguage(AppLanguage lang) {
        currentLang() = lang;
    }

    static std::string getCode(AppLanguage lang) {
        if (lang == AppLanguage::EN) return "en";
        if (lang == AppLanguage::UA) return "ua";
        return "ru";
    }

    static AppLanguage fromCode(const std::string& code) {
        if (code == "en") return AppLanguage::EN;
        if (code == "ua") return AppLanguage::UA;
        return AppLanguage::RU;
    }

    static const char* tr(Str s) {
        static const char* const kTranslations[3][static_cast<int>(Str::COUNT)] = {
            {
                "ПАПКИ",
                "Новая папка...",
                "+ ДОБАВИТЬ ТРЕКИ",
                "НАСТРОЙКИ",
                "РЕЖИМ: ОБЫЧНЫЙ",
                "РЕЖИМ: ПОВТОР [1]",
                "РЕЖИМ: ЦИКЛ [ПАПКА]",
                "РЕЖИМ: ШАФФЛ",
                "СТОП ВСЕ",
                "Название (Двойной клик для смены)",
                "Хоткей",
                "Громкость",
                "Статус",
                "Удал.",
                "[ БИНД ]",
                "ИГРАТЬ",
                "ПАУЗА",
                "ПРОДОЛЖ",
                "СТОП",
                "Загр..",
                "Сейчас: %s",
                "Ожидание",
                "Громк: %.0f%%",
                "Полосы: %.0fpx",
                "Двойной клик — переименование",
                "Коэффициент толщины полос спектра",
                "Настройки",
                "Язык интерфейса:",
                "Закрыть",
                "Удалить папку",
                "Назначение хоткея",
                "Зажмите комбинацию клавиш и отпустите для фиксации:",
                "Очистить",
                "Отмена",
                "[ Нажмите комбинацию... ]",
                "Переместить: %s",
                "Переместить: %s",
                "Основная",
                "Удалить активную папку",
                "Сворачивать в системный трей при закрытии",
                "HotPlay свернут в трей и работает в фоне",
                "В ТРЕЙ",
                "ЭКВАЛАЙЗЕР",
                "Мастер-эквалайзер",
                "Включить эквалайзер",
                "Низкие (150 Гц)",
                "Средние (1 кГц)",
                "Высокие (5 кГц)",
                "Сброс (0 дБ)"
            },
            {
                "FOLDERS",
                "New folder...",
                "+ ADD TRACKS",
                "SETTINGS",
                "MODE: NORMAL",
                "MODE: REPEAT [1]",
                "MODE: LOOP [ALL]",
                "MODE: SHUFFLE",
                "STOP ALL",
                "Title (Double-click to rename)",
                "Hotkey",
                "Volume",
                "State",
                "Del",
                "[ BIND ]",
                "PLAY",
                "PAUSE",
                "RESUME",
                "STOP",
                "Load..",
                "Now: %s",
                "Idle",
                "Vol: %.0f%%",
                "Bars: %.0fpx",
                "Double click — rename track",
                "Visualizer bar width factor",
                "Settings",
                "Interface Language:",
                "Close",
                "Delete folder",
                "Assign Hotkey",
                "Hold key combination and release to bind:",
                "Clear",
                "Cancel",
                "[ Press combination... ]",
                "Move: %s",
                "Move: %s",
                "General",
                "Delete active folder",
                "Minimize to system tray on close",
                "HotPlay minimized to tray and running in background",
                "TO TRAY",
                "EQUALIZER",
                "Master Equalizer",
                "Enable Equalizer",
                "Bass (150 Hz)",
                "Mid (1 kHz)",
                "Treble (5 kHz)",
                "Reset (0 dB)"
            },
            {
                "ПАПКИ",
                "Нова папка...",
                "+ ДОДАТИ ТРЕКИ",
                "НАЛАШТУВАННЯ",
                "РЕЖИМ: ЗВИЧАЙНИЙ",
                "РЕЖИМ: ПОВТОР [1]",
                "РЕЖИМ: ЦИКЛ [ПАПКА]",
                "РЕЖИМ: ШАФФЛ",
                "ЗУПИНИТИ ВСЕ",
                "Назва (Подвійний клік для зміни)",
                "Хоткей",
                "Гучність",
                "Статус",
                "Видал.",
                "[ БІНД ]",
                "ГРАТИ",
                "ПАУЗА",
                "ПРОДОВЖ",
                "СТОП",
                "Заван..",
                "Зараз: %s",
                "Очікування",
                "Гучн: %.0f%%",
                "Смуги: %.0fpx",
                "Подвійний клік — перейменування",
                "Коефіцієнт товщини смуг спектра",
                "Налаштування",
                "Мова інтерфейсу:",
                "Закрити",
                "Видалити папку",
                "Призначення хоткею",
                "Затисніть комбінацію клавіш та відпустіть для фіксації:",
                "Очистити",
                "Скасувати",
                "[ Натисніть комбінацію... ]",
                "Перемістити: %s",
                "Перемістити: %s",
                "Основна",
                "Видалити активну папку",
                "Згортати в системний трей при закритті",
                "HotPlay згорнуто в трей, працює у фоні",
                "В ТРЕЙ",
                "ЕКВАЛАЙЗЕР",
                "Майстер-еквалайзер",
                "Увімкнути еквалайзер",
                "Низькі (150 Гц)",
                "Середні (1 кГц)",
                "Високі (5 кГц)",
                "Скинути (0 дБ)"
            }
        };

        int lang = static_cast<int>(currentLang());
        if (lang < 0 || lang >= static_cast<int>(AppLanguage::COUNT)) lang = 0;

        int idx = static_cast<int>(s);
        if (idx < 0 || idx >= static_cast<int>(Str::COUNT)) return "";

        return kTranslations[lang][idx];
    }
};