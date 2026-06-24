#include "interface.h"
#include "csv_reader.h"
#include "escalonador.h"
#include "memoria.h"
#include "relatorio.h"
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

// Estrutura que mantém os ponteiros para os widgets necessários na execução da simulação
struct AppWidgets {
    GtkWidget* main_window;
    GtkWidget* label_arquivo;
    GtkWidget* entry_mem_fisica;
    GtkWidget* entry_mem_virtual;
    GtkWidget* entry_tam_pagina;
    GtkWidget* combo_escalonador;
    GtkWidget* entry_quantum;
    GtkWidget* combo_substituicao;
    GtkWidget* text_view_relatorio;
    GtkWidget* box_quantum_container; // Contêiner para poder ocultar/exibir o quantum facilmente
};

// Exibe um diálogo de mensagem no centro da janela pai
static void mostrar_mensagem(GtkWindow* parent, const char* titulo, const char* mensagem) {
    GtkWidget* dialog = gtk_message_dialog_new(parent,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               "%s", mensagem);
    gtk_window_set_title(GTK_WINDOW(dialog), titulo);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Callback executado quando o usuário altera o algoritmo no dropdown de escalonamento
static void ao_mudar_escalonador(GtkComboBox* combo, gpointer data) {
    GtkWidget* box_quantum = GTK_WIDGET(data);
    gchar* selecionado = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    
    if (selecionado != nullptr) {
        if (g_strcmp0(selecionado, "Round-Robin") == 0) {
            gtk_widget_show(box_quantum); // Exibe o campo de quantum
        } else {
            gtk_widget_hide(box_quantum); // Oculta para os outros algoritmos
        }
        g_free(selecionado);
    }
}

// Callback executado ao clicar no botão de carregar CSV
static void ao_selecionar_arquivo(GtkButton* botao, gpointer data) {
    AppWidgets* widgets = static_cast<AppWidgets*>(data);
    
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Selecionar Arquivo de Processos (CSV)",
                                                    GTK_WINDOW(widgets->main_window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancelar", GTK_RESPONSE_CANCEL,
                                                    "_Abrir", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    // Configura filtro para arquivos .csv
    GtkFileFilter* filtro = gtk_file_filter_new();
    gtk_file_filter_set_name(filtro, "Arquivos CSV (*.csv)");
    gtk_file_filter_add_pattern(filtro, "*.csv");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filtro);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* caminho = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_label_set_text(GTK_LABEL(widgets->label_arquivo), caminho);
        g_free(caminho);
    }
    
    gtk_widget_destroy(dialog);
}

// Callback principal acionado ao clicar no botão "Simular"
static void ao_clicar_simular(GtkButton* botao, gpointer data) {
    AppWidgets* widgets = static_cast<AppWidgets*>(data);

    // 1. Obter caminho do CSV
    const char* texto_caminho = gtk_label_get_text(GTK_LABEL(widgets->label_arquivo));
    std::string caminho_csv = (texto_caminho != nullptr) ? texto_caminho : "";

    if (caminho_csv.empty() || caminho_csv == "Nenhum arquivo selecionado") {
        mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Arquivo", "Por favor, selecione um arquivo CSV contendo os processos.");
        return;
    }

    // 2. Tentar ler os processos
    std::vector<Processo> processos = ler_csv(caminho_csv);
    if (processos.empty()) {
        mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Dados", "O arquivo CSV não pôde ser lido ou não contém processos válidos.");
        return;
    }

    // 3. Obter parâmetros numéricos de memória
    int mem_fisica = 0;
    int mem_virtual = 0;
    int tam_pagina = 0;
    try {
        mem_fisica = std::stoi(gtk_entry_get_text(GTK_ENTRY(widgets->entry_mem_fisica)));
        mem_virtual = std::stoi(gtk_entry_get_text(GTK_ENTRY(widgets->entry_mem_virtual)));
        tam_pagina = std::stoi(gtk_entry_get_text(GTK_ENTRY(widgets->entry_tam_pagina)));
    } catch (...) {
        mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Entrada", "Insira valores numéricos inteiros válidos nos campos de memória.");
        return;
    }

    if (mem_fisica <= 0 || mem_virtual <= 0 || tam_pagina <= 0) {
        mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Valores", "A memória física, virtual e tamanho da página devem ser maiores que zero.");
        return;
    }

    int num_frames = mem_fisica / tam_pagina;
    if (num_frames <= 0) {
        mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Memória", "O tamanho da página excede a capacidade total da memória física!");
        return;
    }

    // 4. Executar escalonador
    gchar* esc_sel = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widgets->combo_escalonador));
    std::string algoritmo_cpu = (esc_sel != nullptr) ? esc_sel : "";
    g_free(esc_sel);

    std::vector<std::pair<int, int>> linha_tempo;
    if (algoritmo_cpu == "Round-Robin") {
        int quantum = 0;
        try {
            quantum = std::stoi(gtk_entry_get_text(GTK_ENTRY(widgets->entry_quantum)));
        } catch (...) {
            mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Entrada", "Insira um valor numérico inteiro válido para o Quantum.");
            return;
        }
        if (quantum <= 0) {
            mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Valores", "O Quantum deve ser maior que zero.");
            return;
        }
        linha_tempo = escalonamento_round_robin(processos, quantum);
    } else if (algoritmo_cpu == "SJF Preemptivo") {
        linha_tempo = escalonamento_sjf_preemptivo(processos);
    } else if (algoritmo_cpu == "Prioridade Preemptiva") {
        linha_tempo = escalonamento_prioridade_preemptivo(processos);
    } else {
        mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Algoritmo", "Algoritmo de escalonamento desconhecido.");
        return;
    }

    // 5. Executar simulação de paginação
    gchar* sub_sel = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widgets->combo_substituicao));
    std::string politica_paginas = (sub_sel != nullptr) ? sub_sel : "";
    g_free(sub_sel);

    int page_faults = 0;
    if (politica_paginas == "FIFO") {
        page_faults = simular_fifo(num_frames, tam_pagina, processos, linha_tempo);
    } else if (politica_paginas == "LRU") {
        page_faults = simular_lru(num_frames, tam_pagina, processos, linha_tempo);
    } else if (politica_paginas == "Ótimo") {
        page_faults = simular_otimo(num_frames, tam_pagina, processos, linha_tempo);
    } else {
        mostrar_mensagem(GTK_WINDOW(widgets->main_window), "Erro de Memória", "Política de substituição de páginas desconhecida.");
        return;
    }

    // 6. Gerar e exibir relatório na área de texto
    std::string relatorio_txt = gerar_relatorio_string(processos, linha_tempo, page_faults);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->text_view_relatorio));
    gtk_text_buffer_set_text(buffer, relatorio_txt.c_str(), -1);
}

// Liberação de recursos quando a janela do app é fechada
static void ao_destruir_janela(GtkWidget* widget, gpointer data) {
    AppWidgets* widgets = static_cast<AppWidgets*>(data);
    delete widgets;
    gtk_main_quit();
}

// Aplica estilo CSS customizado (Catppuccin Dark) para a janela
static void aplicar_estilo_css() {
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Segoe UI', sans-serif; }\n"
        "grid { padding: 15px; }\n"
        "label { font-weight: bold; padding: 4px; color: #cdd6f4; font-size: 10pt; }\n"
        "button { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; padding: 8px 16px; margin: 4px; font-weight: bold; }\n"
        "button:hover { background-color: #45475a; color: #f5c2e7; }\n"
        "entry { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; padding: 6px; font-size: 10pt; }\n"
        "combobox { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; }\n"
        "textview text { background-color: #11111b; color: #a6e3a1; font-family: 'Consolas', 'Courier New', monospace; font-size: 11pt; padding: 12px; }\n"
        "scrolledwindow { border: 1px solid #45475a; border-radius: 6px; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

void inicializar_interface(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    aplicar_estilo_css();

    AppWidgets* widgets = new AppWidgets();

    // Criação da Janela Principal
    widgets->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widgets->main_window), "Simulador de SO - CPU & Memória Virtual");
    gtk_window_set_default_size(GTK_WINDOW(widgets->main_window), 850, 650);
    gtk_window_set_position(GTK_WINDOW(widgets->main_window), GTK_WIN_POS_CENTER);

    // Box vertical que contém todo o layout
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 15);
    gtk_container_add(GTK_CONTAINER(widgets->main_window), main_box);

    // Título Principal
    GtkWidget* label_titulo = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_titulo), "<span size='xx-large' weight='bold' foreground='#cba6f7'>Simulador de Sistemas Operacionais</span>");
    gtk_box_pack_start(GTK_BOX(main_box), label_titulo, FALSE, FALSE, 5);

    // Grid para organizar os parâmetros
    GtkWidget* grid_parametros = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_parametros), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid_parametros), 15);
    gtk_box_pack_start(GTK_BOX(main_box), grid_parametros, FALSE, FALSE, 5);

    // 1. Seletor de Arquivos CSV
    GtkWidget* btn_arquivo = gtk_button_new_with_label("Selecionar CSV...");
    widgets->label_arquivo = gtk_label_new("Nenhum arquivo selecionado");
    gtk_label_set_xalign(GTK_LABEL(widgets->label_arquivo), 0.0);
    
    gtk_grid_attach(GTK_GRID(grid_parametros), btn_arquivo, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_parametros), widgets->label_arquivo, 1, 0, 1, 1);

    // 2. Parâmetros de Memória Física
    GtkWidget* lbl_mem_fisica = gtk_label_new("Memória Física Total (MB):");
    gtk_label_set_xalign(GTK_LABEL(lbl_mem_fisica), 0.0);
    widgets->entry_mem_fisica = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_mem_fisica), "256");
    
    gtk_grid_attach(GTK_GRID(grid_parametros), lbl_mem_fisica, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_parametros), widgets->entry_mem_fisica, 1, 1, 1, 1);

    // 3. Parâmetros de Memória Virtual
    GtkWidget* lbl_mem_virtual = gtk_label_new("Memória Virtual Total (MB):");
    gtk_label_set_xalign(GTK_LABEL(lbl_mem_virtual), 0.0);
    widgets->entry_mem_virtual = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_mem_virtual), "1024");
    
    gtk_grid_attach(GTK_GRID(grid_parametros), lbl_mem_virtual, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_parametros), widgets->entry_mem_virtual, 1, 2, 1, 1);

    // 4. Tamanho da Página
    GtkWidget* lbl_tam_pagina = gtk_label_new("Tamanho da Página (MB):");
    gtk_label_set_xalign(GTK_LABEL(lbl_tam_pagina), 0.0);
    widgets->entry_tam_pagina = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_tam_pagina), "4");
    
    gtk_grid_attach(GTK_GRID(grid_parametros), lbl_tam_pagina, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_parametros), widgets->entry_tam_pagina, 1, 3, 1, 1);

    // 5. Seletor de Algoritmo de Escalonamento CPU
    GtkWidget* lbl_escalonador = gtk_label_new("Algoritmo CPU:");
    gtk_label_set_xalign(GTK_LABEL(lbl_escalonador), 0.0);
    widgets->combo_escalonador = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->combo_escalonador), "Round-Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->combo_escalonador), "SJF Preemptivo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->combo_escalonador), "Prioridade Preemptiva");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->combo_escalonador), 0);
    
    gtk_grid_attach(GTK_GRID(grid_parametros), lbl_escalonador, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_parametros), widgets->combo_escalonador, 1, 4, 1, 1);

    // 6. Contêiner e entrada para o Quantum (Apenas visível no Round-Robin)
    widgets->box_quantum_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget* lbl_quantum = gtk_label_new("Quantum (ticks):");
    widgets->entry_quantum = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_quantum), "2");
    gtk_box_pack_start(GTK_BOX(widgets->box_quantum_container), lbl_quantum, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(widgets->box_quantum_container), widgets->entry_quantum, TRUE, TRUE, 0);
    
    gtk_grid_attach(GTK_GRID(grid_parametros), widgets->box_quantum_container, 0, 5, 2, 1);

    // 7. Seletor de Política de Memória Virtual
    GtkWidget* lbl_substituicao = gtk_label_new("Substituição de Página:");
    gtk_label_set_xalign(GTK_LABEL(lbl_substituicao), 0.0);
    widgets->combo_substituicao = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->combo_substituicao), "FIFO");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->combo_substituicao), "LRU");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->combo_substituicao), "Ótimo");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->combo_substituicao), 0);
    
    gtk_grid_attach(GTK_GRID(grid_parametros), lbl_substituicao, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_parametros), widgets->combo_substituicao, 1, 6, 1, 1);

    // Configuração dos Sinais da Interface
    g_signal_connect(widgets->combo_escalonador, "changed", G_CALLBACK(ao_mudar_escalonador), widgets->box_quantum_container);
    g_signal_connect(btn_arquivo, "clicked", G_CALLBACK(ao_selecionar_arquivo), widgets);

    // 8. Botão Simular
    GtkWidget* btn_simular = gtk_button_new_with_label("Simular Execução");
    g_signal_connect(btn_simular, "clicked", G_CALLBACK(ao_clicar_simular), widgets);
    gtk_box_pack_start(GTK_BOX(main_box), btn_simular, FALSE, FALSE, 5);

    // 9. Área do Relatório
    GtkWidget* label_relatorio_header = gtk_label_new("Relatório de Simulação:");
    gtk_label_set_xalign(GTK_LABEL(label_relatorio_header), 0.0);
    gtk_box_pack_start(GTK_BOX(main_box), label_relatorio_header, FALSE, FALSE, 0);

    widgets->text_view_relatorio = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->text_view_relatorio), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(widgets->text_view_relatorio), TRUE);
    
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), widgets->text_view_relatorio);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled_window, TRUE, TRUE, 5);

    // Sinal de encerramento da aplicação
    g_signal_connect(widgets->main_window, "destroy", G_CALLBACK(ao_destruir_janela), widgets);

    // Inicialização da exibição
    gtk_widget_show_all(widgets->main_window);
    
    // Executa a main loop do GTK
    gtk_main();
}
