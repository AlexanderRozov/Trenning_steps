#include "lang.h"

#include <string.h>

static int g_lang = LANG_RU;

static const char *const lang_codes[LANG_COUNT] = {
    "ru", "en", "de", "jv", "fr", "zh", "pt", "es"
};

static const char *const lang_names[LANG_COUNT] = {
    "Русский", "English", "Deutsch", "Basa Jawa",
    "Français", "简体中文", "Português", "Español"
};

/* Порядок столбцов: RU, EN, DE, JV, FR, ZH, PT, ES */
static const char *const strings[S_COUNT][LANG_COUNT] = {
    [S_APP_TITLE] = {
        "tREND — трекер шагов", "tREND — step tracker", "tREND — Schritt-Tracker",
        "tREND — pelacak langkah", "tREND — suivi de pas", "tREND — 步数追踪器",
        "tREND — rastreador de passos", "tREND — rastreador de pasos"
    },
    [S_TODAY] = {
        "Сегодня", "Today", "Heute", "Dina iki", "Aujourd'hui", "今天", "Hoje", "Hoy"
    },
    [S_TODAY_LINE] = {
        "%s — цель %d шагов: %s", "%s — goal %d steps: %s", "%s — Ziel %d Schritte: %s",
        "%s — target %d langkah: %s", "%s — objectif %d pas : %s", "%s — 目标 %d 步：%s",
        "%s — meta de %d passos: %s", "%s — objetivo %d pasos: %s"
    },
    [S_MARK_DONE] = {
        "Отметить выполненным", "Mark as done", "Als erledigt markieren",
        "Tandhai wis rampung", "Marquer comme fait", "标记为已完成",
        "Marcar como concluído", "Marcar como hecho"
    },
    [S_NO_TODAY] = {
        "На сегодня тренировки нет", "No workout today", "Heute kein Training",
        "Ora ana latihan dina iki", "Pas d'entraînement aujourd'hui", "今天没有训练",
        "Sem treino hoje", "No hay entrenamiento hoy"
    },
    [S_SCHEDULE] = {
        "Расписание", "Schedule", "Zeitplan", "Jadwal", "Planning", "日程表",
        "Cronograma", "Horario"
    },
    [S_GENERATE] = {
        "Сгенерировать 7 дней", "Generate 7 days", "7 Tage generieren",
        "Gawe 7 dina", "Générer 7 jours", "生成 7 天", "Gerar 7 dias", "Generar 7 días"
    },
    [S_REFRESH] = {
        "Обновить", "Refresh", "Aktualisieren", "Anyari", "Actualiser", "刷新",
        "Atualizar", "Actualizar"
    },
    [S_COL_DATE] = {
        "Дата", "Date", "Datum", "Tanggal", "Date", "日期", "Data", "Fecha"
    },
    [S_COL_STEPS] = {
        "Шаги", "Steps", "Schritte", "Langkah", "Pas", "步数", "Passos", "Pasos"
    },
    [S_COL_STATUS] = {
        "Статус", "Status", "Status", "Status", "Statut", "状态", "Estado", "Estado"
    },
    [S_DONE] = {
        "выполнено", "done", "erledigt", "rampung", "fait", "已完成",
        "concluído", "hecho"
    },
    [S_NOT_DONE] = {
        "не выполнено", "not done", "offen", "durung", "non fait", "未完成",
        "pendente", "pendiente"
    },
    [S_BTN_DONE] = {
        "Готово", "Done", "Fertig", "Rampung", "Fait", "完成", "Feito", "Hecho"
    },
    [S_BTN_RESET] = {
        "Сброс", "Reset", "Zurücksetzen", "Reset", "Réinitialiser", "重置",
        "Repor", "Restablecer"
    },
    [S_BTN_DELETE] = {
        "Удалить", "Delete", "Löschen", "Busak", "Supprimer", "删除",
        "Eliminar", "Eliminar"
    },
    [S_SETTINGS] = {
        "Настройки", "Settings", "Einstellungen", "Setelan", "Paramètres", "设置",
        "Definições", "Ajustes"
    },
    [S_START_STEPS] = {
        "Начальные шаги", "Starting steps", "Start-Schritte", "Langkah wiwitan",
        "Pas initiaux", "起始步数", "Passos iniciais", "Pasos iniciales"
    },
    [S_INCREMENT] = {
        "Прибавка в день", "Daily increment", "Tägliche Steigerung",
        "Tambahan saben dina", "Incrément quotidien", "每日增量",
        "Incremento diário", "Incremento diario"
    },
    [S_NOTIFY_HOUR] = {
        "Час напоминания", "Reminder hour", "Erinnerungsstunde", "Jam pangeling",
        "Heure du rappel", "提醒小时", "Hora do lembrete", "Hora del recordatorio"
    },
    [S_NOTIFY_MINUTE] = {
        "Минута напоминания", "Reminder minute", "Erinnerungsminute", "Menit pangeling",
        "Minute du rappel", "提醒分钟", "Minuto do lembrete", "Minuto del recordatorio"
    },
    [S_NOTIFY_PERIOD] = {
        "Периодичность (раз в N дней)", "Frequency (every N days)",
        "Häufigkeit (alle N Tage)", "Kerep (saben N dina)",
        "Fréquence (tous les N jours)", "频率（每 N 天）",
        "Frequência (a cada N dias)", "Frecuencia (cada N días)"
    },
    [S_AUTOSTART] = {
        "Запускать при старте Windows", "Run at Windows startup",
        "Beim Windows-Start ausführen", "Jalanake nalika Windows miwiti",
        "Lancer au démarrage de Windows", "开机自启动",
        "Iniciar com o Windows", "Iniciar con Windows"
    },
    [S_LANGUAGE] = {
        "Язык", "Language", "Sprache", "Basa", "Langue", "语言", "Idioma", "Idioma"
    },
    [S_MSG_ADDED] = {
        "Добавлено дней: %d", "Days added: %d", "Tage hinzugefügt: %d",
        "Dina ditambahake: %d", "Jours ajoutés : %d", "已添加天数：%d",
        "Dias adicionados: %d", "Días añadidos: %d"
    },
    [S_MSG_GEN_ERROR] = {
        "Ошибка генерации", "Generation error", "Generierungsfehler",
        "Kesalahan generasi", "Erreur de génération", "生成错误",
        "Erro ao gerar", "Error al generar"
    },
    [S_MSG_READ_ERROR] = {
        "Ошибка чтения базы данных", "Database read error", "Datenbank-Lesefehler",
        "Kesalahan maca basis data", "Erreur de lecture de la base", "数据库读取错误",
        "Erro de leitura da base de dados", "Error de lectura de la base de datos"
    },
    [S_MSG_TRAINING_DONE] = {
        "Тренировка выполнена!", "Workout done!", "Training erledigt!",
        "Latihan rampung!", "Entraînement terminé !", "训练完成！",
        "Treino concluído!", "¡Entrenamiento hecho!"
    },
    [S_MSG_MARKED_DONE] = {
        "%s — выполнено", "%s — done", "%s — erledigt", "%s — rampung",
        "%s — fait", "%s — 已完成", "%s — concluído", "%s — hecho"
    },
    [S_MSG_MARKED_UNDONE] = {
        "%s — не выполнено", "%s — not done", "%s — offen", "%s — durung",
        "%s — non fait", "%s — 未完成", "%s — pendente", "%s — pendiente"
    },
    [S_MSG_DELETED] = {
        "Запись %s удалена", "Entry %s deleted", "Eintrag %s gelöscht",
        "Cathetan %s dihapus", "Entrée %s supprimée", "记录 %s 已删除",
        "Registo %s eliminado", "Registro %s eliminado"
    },
    [S_MSG_AUTOSTART_ON] = {
        "Автозагрузка включена", "Autostart enabled", "Autostart aktiviert",
        "Autostart aktif", "Démarrage automatique activé", "已启用开机自启",
        "Arranque automático ativado", "Inicio automático activado"
    },
    [S_MSG_AUTOSTART_OFF] = {
        "Автозагрузка выключена", "Autostart disabled", "Autostart deaktiviert",
        "Autostart mati", "Démarrage automatique désactivé", "已禁用开机自启",
        "Arranque automático desativado", "Inicio automático desactivado"
    },
    [S_MSG_AUTOSTART_ERROR] = {
        "Ошибка настройки автозагрузки", "Autostart setup error", "Autostart-Fehler",
        "Kesalahan setelan autostart", "Erreur de configuration du démarrage",
        "自启动设置错误", "Erro na configuração do arranque",
        "Error de configuración del inicio"
    },
    [S_NOTIFY_TITLE] = {
        "tREND — время тренировки", "tREND — workout time", "tREND — Trainingszeit",
        "tREND — wektu latihan", "tREND — heure de l'entraînement", "tREND — 训练时间",
        "tREND — hora do treino", "tREND — hora de entrenar"
    },
    [S_NOTIFY_TEXT] = {
        "Цель на сегодня: %d шагов. Не забудьте пройтись!",
        "Today's goal: %d steps. Don't forget to walk!",
        "Heutiges Ziel: %d Schritte. Nicht vergessen zu gehen!",
        "Target dina iki: %d langkah. Aja lali mlaku!",
        "Objectif du jour : %d pas. N'oubliez pas de marcher !",
        "今日目标：%d 步。别忘了走路！",
        "Meta de hoje: %d passos. Não se esqueça de caminhar!",
        "Objetivo de hoy: %d pasos. ¡No olvides caminar!"
    },
    [S_DB_CONNECTED] = {
        "База данных '%s' подключена.", "Database '%s' connected.",
        "Datenbank '%s' verbunden.", "Basis data '%s' kasil disambung.",
        "Base de données « %s » connectée.", "数据库 '%s' 已连接。",
        "Base de dados '%s' ligada.", "Base de datos '%s' conectada."
    },
    [S_MENU_GENERATE] = {
        "Сгенерировать расписание", "Generate schedule", "Zeitplan generieren",
        "Gawe jadwal", "Générer le planning", "生成日程",
        "Gerar cronograma", "Generar horario"
    },
    [S_MENU_LIST] = {
        "Показать расписание", "Show schedule", "Zeitplan anzeigen",
        "Tampilake jadwal", "Afficher le planning", "显示日程",
        "Mostrar cronograma", "Mostrar horario"
    },
    [S_MENU_DONE] = {
        "Отметить выполненным", "Mark as done", "Als erledigt markieren",
        "Tandhai rampung", "Marquer comme fait", "标记完成",
        "Marcar como feito", "Marcar como hecho"
    },
    [S_MENU_RESET] = {
        "Сбросить статус", "Reset status", "Status zurücksetzen",
        "Reset status", "Réinitialiser le statut", "重置状态",
        "Repor estado", "Restablecer estado"
    },
    [S_MENU_DELETE] = {
        "Удалить запись", "Delete entry", "Eintrag löschen",
        "Busak cathetan", "Supprimer l'entrée", "删除记录",
        "Eliminar registo", "Eliminar registro"
    },
    [S_MENU_LANGUAGE] = {
        "Язык", "Language", "Sprache", "Basa", "Langue", "语言", "Idioma", "Idioma"
    },
    [S_MENU_EXIT] = {
        "Выход", "Exit", "Beenden", "Metu", "Quitter", "退出", "Sair", "Salir"
    },
    [S_MENU_CHOICE] = {
        "Выбор: ", "Choice: ", "Auswahl: ", "Pilihan: ", "Choix : ", "选择：",
        "Escolha: ", "Opción: "
    },
    [S_INVALID_INPUT] = {
        "Некорректный ввод, попробуйте ещё раз.", "Invalid input, try again.",
        "Ungültige Eingabe, erneut versuchen.", "Input salah, coba maneh.",
        "Entrée invalide, réessayez.", "输入无效，请重试。",
        "Entrada inválida, tente novamente.", "Entrada no válida, inténtelo de nuevo."
    },
    [S_PROMPT_DAYS] = {
        "Сколько дней сгенерировать: ", "How many days to generate: ",
        "Wie viele Tage generieren: ", "Pira dina sing digawe: ",
        "Combien de jours générer : ", "要生成多少天：",
        "Quantos dias gerar: ", "Cuántos días generar: "
    },
    [S_PROMPT_ID] = {
        "Введите ID записи: ", "Enter entry ID: ", "Eintrags-ID eingeben: ",
        "Lebokake ID cathetan: ", "Entrez l'ID de l'entrée : ", "输入记录 ID：",
        "Introduza o ID do registo: ", "Introduzca el ID del registro: "
    },
    [S_RECORD_SET] = {
        "Запись %d: %s.", "Entry %d: %s.", "Eintrag %d: %s.", "Cathetan %d: %s.",
        "Entrée %d : %s.", "记录 %d：%s。", "Registo %d: %s.", "Registro %d: %s."
    },
    [S_RECORD_DELETED] = {
        "Запись %d удалена.", "Entry %d deleted.", "Eintrag %d gelöscht.",
        "Cathetan %d dihapus.", "Entrée %d supprimée.", "记录 %d 已删除。",
        "Registo %d eliminado.", "Registro %d eliminado."
    },
    [S_RECORD_NOT_FOUND] = {
        "Запись с ID %d не найдена.", "Entry with ID %d not found.",
        "Eintrag mit ID %d nicht gefunden.", "Cathetan kanthi ID %d ora ditemokake.",
        "Entrée avec l'ID %d introuvable.", "未找到 ID 为 %d 的记录。",
        "Registo com ID %d não encontrado.", "Registro con ID %d no encontrado."
    },
    [S_BYE] = {
        "До свидания!", "Goodbye!", "Auf Wiedersehen!", "Katur nuwun!",
        "Au revoir !", "再见！", "Adeus!", "¡Adiós!"
    },
    [S_LIST_HEADER] = {
        "--- Расписание тренировок ---", "--- Workout schedule ---",
        "--- Trainingsplan ---", "--- Jadwal latihan ---",
        "--- Planning d'entraînement ---", "--- 训练日程 ---",
        "--- Cronograma de treinos ---", "--- Horario de entrenamientos ---"
    },
    [S_EMPTY] = {
        "Расписание пусто. Сгенерируйте его (пункт 1).",
        "Schedule is empty. Generate it (option 1).",
        "Zeitplan ist leer. Generieren Sie ihn (Option 1).",
        "Jadwal kosong. Gawe dhisik (pilihan 1).",
        "Le planning est vide. Générez-le (option 1).",
        "日程为空。请先生成（选项 1）。",
        "Cronograma vazio. Gere-o (opção 1).",
        "El horario está vacío. Genérelo (opción 1)."
    },
    [S_TOTAL] = {
        "Всего записей: %d", "Total entries: %d", "Einträge gesamt: %d",
        "Gunggung cathetan: %d", "Total d'entrées : %d", "总记录数：%d",
        "Total de registos: %d", "Total de registros: %d"
    },
    [S_CHOOSE_LANG] = {
        "Выберите язык:", "Choose language:", "Sprache wählen:", "Pilih basa:",
        "Choisissez la langue :", "选择语言：", "Escolha o idioma:", "Elija el idioma:"
    },
};

void lang_set(int lang) {
    if (lang >= 0 && lang < LANG_COUNT)
        g_lang = lang;
}

int lang_get(void) {
    return g_lang;
}

int lang_from_code(const char *code) {
    for (int i = 0; i < LANG_COUNT; i++) {
        if (strcmp(lang_codes[i], code) == 0)
            return i;
    }
    return -1;
}

const char *lang_code(int lang) {
    if (lang < 0 || lang >= LANG_COUNT) return lang_codes[LANG_RU];
    return lang_codes[lang];
}

const char *lang_name(int lang) {
    if (lang < 0 || lang >= LANG_COUNT) return lang_names[LANG_RU];
    return lang_names[lang];
}

const char *tr(int id) {
    if (id < 0 || id >= S_COUNT) return "?";
    return strings[id][g_lang];
}
