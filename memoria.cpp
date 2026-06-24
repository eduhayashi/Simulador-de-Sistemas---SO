#include "memoria.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <iostream>

// Estrutura auxiliar para identificar uma página no espaço de endereçamento global do simulador.
struct Pagina {
    int id_processo;
    int numero_pagina;

    bool operator==(const Pagina& outra) const {
        return id_processo == outra.id_processo && numero_pagina == outra.numero_pagina;
    }
};

// Função auxiliar que gera a sequência cronológica de acessos a páginas com base
// na execução da CPU (linha do tempo do escalonador).
static std::vector<Pagina> gerar_referencias_pagina(const std::vector<Processo>& processos, 
                                                    const std::vector<std::pair<int, int>>& linha_tempo, 
                                                    int tamanho_pagina_mb) {
    std::vector<Pagina> referencias;
    if (tamanho_pagina_mb <= 0) return referencias;

    // Mapa para busca rápida do processo a partir de seu ID
    std::unordered_map<int, Processo> mapa_processos;
    for (const auto& p : processos) {
        mapa_processos[p.id] = p;
    }

    // Acompanha o número de ticks de clock acumulado que cada processo já rodou na CPU
    std::unordered_map<int, int> tempo_executado_processo;

    for (const auto& tick : linha_tempo) {
        int id_proc = tick.first;
        if (id_proc == -1) {
            continue; // CPU ociosa, não há acesso à memória neste instante
        }

        auto it = mapa_processos.find(id_proc);
        if (it == mapa_processos.end()) {
            continue;
        }

        const auto& p = it->second;

        // Calcula a quantidade de páginas virtuais que o processo possui
        int num_paginas = (p.memoria_mb + tamanho_pagina_mb - 1) / tamanho_pagina_mb;
        if (num_paginas <= 0) {
            num_paginas = 1;
        }

        // Determina qual página está sendo acessada.
        // Simulamos o comportamento de varrer as páginas sequencialmente à medida que o processo executa.
        int tick_atual = tempo_executado_processo[id_proc]++;
        int pag_acessada = tick_atual % num_paginas;

        referencias.push_back({id_proc, pag_acessada});
    }

    return referencias;
}

int simular_fifo(int num_frames, int tamanho_pagina_mb, 
                 const std::vector<Processo>& processos, 
                 const std::vector<std::pair<int, int>>& linha_tempo) {
    
    std::vector<Pagina> referencias = gerar_referencias_pagina(processos, linha_tempo, tamanho_pagina_mb);
    if (num_frames <= 0 || referencias.empty()) return 0;

    std::vector<Pagina> frames;
    std::queue<Pagina> fila_fifo;
    int page_faults = 0;

    for (const auto& pag : referencias) {
        // Verifica se a página já está mapeada em algum frame físico (Hit)
        auto it = std::find(frames.begin(), frames.end(), pag);
        if (it != frames.end()) {
            continue;
        }

        // Ocorreu um Page Fault!
        page_faults++;

        if (frames.size() < static_cast<size_t>(num_frames)) {
            frames.push_back(pag);
            fila_fifo.push(pag);
        } else {
            // Remove a página mais antiga carregada na memória (FIFO)
            Pagina antiga = fila_fifo.front();
            fila_fifo.pop();

            auto it_rem = std::find(frames.begin(), frames.end(), antiga);
            if (it_rem != frames.end()) {
                frames.erase(it_rem);
            }

            frames.push_back(pag);
            fila_fifo.push(pag);
        }
    }

    return page_faults;
}

int simular_lru(int num_frames, int tamanho_pagina_mb, 
                const std::vector<Processo>& processos, 
                const std::vector<std::pair<int, int>>& linha_tempo) {
    
    std::vector<Pagina> referencias = gerar_referencias_pagina(processos, linha_tempo, tamanho_pagina_mb);
    if (num_frames <= 0 || referencias.empty()) return 0;

    std::vector<Pagina> frames;
    // Armazena o instante de tempo do último acesso correspondente a cada página no vetor de frames
    std::vector<int> ultimo_acesso;
    int page_faults = 0;

    for (int t = 0; t < static_cast<int>(referencias.size()); ++t) {
        const auto& pag = referencias[t];

        auto it = std::find(frames.begin(), frames.end(), pag);
        if (it != frames.end()) {
            // Hit! Atualiza o instante do último acesso
            int idx = std::distance(frames.begin(), it);
            ultimo_acesso[idx] = t;
            continue;
        }

        // Ocorreu um Page Fault!
        page_faults++;

        if (frames.size() < static_cast<size_t>(num_frames)) {
            frames.push_back(pag);
            ultimo_acesso.push_back(t);
        } else {
            // Procura a página menos recentemente usada (menor valor de instante de acesso)
            int min_idx = 0;
            int min_t = ultimo_acesso[0];
            for (size_t i = 1; i < frames.size(); ++i) {
                if (ultimo_acesso[i] < min_t) {
                    min_t = ultimo_acesso[i];
                    min_idx = i;
                }
            }

            // Substitui a página selecionada pelo LRU
            frames[min_idx] = pag;
            ultimo_acesso[min_idx] = t;
        }
    }

    return page_faults;
}

int simular_otimo(int num_frames, int tamanho_pagina_mb, 
                  const std::vector<Processo>& processos, 
                  const std::vector<std::pair<int, int>>& linha_tempo) {
    
    std::vector<Pagina> referencias = gerar_referencias_pagina(processos, linha_tempo, tamanho_pagina_mb);
    if (num_frames <= 0 || referencias.empty()) return 0;

    std::vector<Pagina> frames;
    int page_faults = 0;

    for (int t = 0; t < static_cast<int>(referencias.size()); ++t) {
        const auto& pag = referencias[t];

        auto it = std::find(frames.begin(), frames.end(), pag);
        if (it != frames.end()) {
            continue; // Hit
        }

        // Ocorreu um Page Fault!
        page_faults++;

        if (frames.size() < static_cast<size_t>(num_frames)) {
            frames.push_back(pag);
        } else {
            // Procura o frame com a página que demorará mais tempo para ser usada no futuro (ou nunca mais será)
            int idx_para_substituir = -1;
            int maior_futuro = -1;

            for (size_t i = 0; i < frames.size(); ++i) {
                const auto& pag_no_frame = frames[i];
                int proximo_uso = 1e9; // Representa infinito caso não seja usada novamente

                // Varre a sequência futura
                for (int j = t + 1; j < static_cast<int>(referencias.size()); ++j) {
                    if (referencias[j] == pag_no_frame) {
                        proximo_uso = j;
                        break;
                    }
                }

                if (proximo_uso > maior_futuro) {
                    maior_futuro = proximo_uso;
                    idx_para_substituir = i;
                }
            }

            // Substitui a página ótima escolhida
            if (idx_para_substituir != -1) {
                frames[idx_para_substituir] = pag;
            }
        }
    }

    return page_faults;
}
