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

// Entrada na Tabela de Páginas de um processo (Demand Paging)
struct EntradaTabelaPagina {
    int frame_fisico = -1;
    bool presente = false;
};

// Função auxiliar que gera a sequência cronológica de acessos a páginas com base
// na execução da CPU (linha do tempo do escalonador).
//
// Modelo de acesso: localidade de referência (working-set).
// A cada tick o processo acessa sua "página de trabalho atual" com 70% de
// probabilidade (hit de localidade) ou salta para uma página vizinha/aleatória
// com 30% de probabilidade. Isso cria a variação necessária para que FIFO,
// LRU e Ótimo divirjam nos seus totais de page faults.
//
// Para garantir resultados DETERMINÍSTICOS e reproduzíveis (o mesmo CSV sempre
// gera a mesma sequência de referências), usamos um gerador de números
// pseudo-aleatórios com semente fixa derivada do ID e da memória do processo.
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

    // Estado do gerador LCG por processo (semente = id * primo + memoria)
    std::unordered_map<int, unsigned int> rng_state;
    // Página de trabalho atual (working-set page) por processo
    std::unordered_map<int, int> pagina_atual;

    // Inicializa sementes determinísticas por processo
    for (const auto& p : processos) {
        rng_state[p.id] = static_cast<unsigned int>(p.id * 1103515245 + p.memoria_mb * 31337);
        pagina_atual[p.id] = 0;
    }

    // Gera um inteiro pseudo-aleatório pelo método LCG (Linear Congruential Generator)
    auto lcg_rand = [](unsigned int& estado) -> unsigned int {
        estado = estado * 1664525u + 1013904223u; // Parâmetros de Knuth
        return estado;
    };

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

        int num_paginas = (p.memoria_mb + tamanho_pagina_mb - 1) / tamanho_pagina_mb;
        if (num_paginas <= 0) num_paginas = 1;

        unsigned int r = lcg_rand(rng_state[id_proc]);
        int& pag = pagina_atual[id_proc];

        // 70% de chance: permanece na mesma página (localidade temporal)
        // 15% de chance: avança para a página seguinte (localidade espacial)
        // 15% de chance: salta para uma página distante (acesso não-local)
        unsigned int escolha = r % 100;
        if (escolha < 70) {
            // Mantém página atual — sem mudança
        } else if (escolha < 85) {
            pag = (pag + 1) % num_paginas; // Avanço sequencial
        } else {
            // Salto para página distante usando segundo número aleatório
            unsigned int r2 = lcg_rand(rng_state[id_proc]);
            pag = static_cast<int>(r2 % static_cast<unsigned int>(num_paginas));
        }

        referencias.push_back({id_proc, pag});
    }

    return referencias;
}


int simular_fifo(int num_frames, int tamanho_pagina_mb, 
                 const std::vector<Processo>& processos, 
                 const std::vector<std::pair<int, int>>& linha_tempo) {
    
    std::vector<Pagina> referencias = gerar_referencias_pagina(processos, linha_tempo, tamanho_pagina_mb);
    if (num_frames <= 0 || referencias.empty()) return 0;

    // Inicializa a Tabela de Páginas de cada processo com o tamanho adequado (Demand Paging)
    std::unordered_map<int, std::vector<EntradaTabelaPagina>> tabelas_paginas;
    for (const auto& p : processos) {
        int num_paginas = (p.memoria_mb + tamanho_pagina_mb - 1) / tamanho_pagina_mb;
        if (num_paginas <= 0) num_paginas = 1;
        tabelas_paginas[p.id] = std::vector<EntradaTabelaPagina>(num_paginas);
    }

    std::vector<Pagina> frames;
    std::queue<Pagina> fila_fifo;
    int page_faults = 0;

    for (const auto& pag : referencias) {
        int id_proc = pag.id_processo;
        int num_pag = pag.numero_pagina;

        // 1 & 2. CPU pede a página e consulta a tabela de páginas
        if (tabelas_paginas[id_proc][num_pag].presente) {
            // 3. Hit! No FIFO, não precisa fazer nada especial
            continue;
        }

        // 4. Se presente for false, ocorreu um Page Fault
        page_faults++;

        // 5. Se houver espaço livre na RAM, aloque a página em um frame vazio e atualize a tabela
        if (frames.size() < static_cast<size_t>(num_frames)) {
            int frame_idx = static_cast<int>(frames.size());
            frames.push_back(pag);
            
            tabelas_paginas[id_proc][num_pag].presente = true;
            tabelas_paginas[id_proc][num_pag].frame_fisico = frame_idx;
            
            fila_fifo.push(pag);
        } else {
            // 6 & 7. RAM cheia: roda o algoritmo FIFO para escolher página vítima
            Pagina antiga = fila_fifo.front();
            fila_fifo.pop();

            // Resgata o frame da vítima a partir de sua tabela de páginas
            int victim_frame = tabelas_paginas[antiga.id_processo][antiga.numero_pagina].frame_fisico;

            // Marca a vítima como ausente da RAM
            tabelas_paginas[antiga.id_processo][antiga.numero_pagina].presente = false;
            tabelas_paginas[antiga.id_processo][antiga.numero_pagina].frame_fisico = -1;

            // Insere a nova página no frame liberado e atualiza a sua tabela de páginas
            frames[victim_frame] = pag;
            tabelas_paginas[id_proc][num_pag].presente = true;
            tabelas_paginas[id_proc][num_pag].frame_fisico = victim_frame;

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

    // Inicializa a Tabela de Páginas de cada processo com o tamanho adequado (Demand Paging)
    std::unordered_map<int, std::vector<EntradaTabelaPagina>> tabelas_paginas;
    for (const auto& p : processos) {
        int num_paginas = (p.memoria_mb + tamanho_pagina_mb - 1) / tamanho_pagina_mb;
        if (num_paginas <= 0) num_paginas = 1;
        tabelas_paginas[p.id] = std::vector<EntradaTabelaPagina>(num_paginas);
    }

    std::vector<Pagina> frames;
    // Armazena o instante de tempo do último acesso correspondente a cada frame físico
    std::vector<int> ultimo_acesso;
    int page_faults = 0;

    for (int t = 0; t < static_cast<int>(referencias.size()); ++t) {
        const auto& pag = referencias[t];
        int id_proc = pag.id_processo;
        int num_pag = pag.numero_pagina;

        // 1 & 2. CPU pede a página e consulta a tabela de páginas
        if (tabelas_paginas[id_proc][num_pag].presente) {
            // 3. Hit! Como é LRU, precisamos atualizar o tempo de último acesso daquele frame
            int frame_idx = tabelas_paginas[id_proc][num_pag].frame_fisico;
            ultimo_acesso[frame_idx] = t;
            continue;
        }

        // 4. Se presente for false, ocorreu um Page Fault
        page_faults++;

        // 5. Se houver espaço livre na RAM, aloque a página em um frame vazio
        if (frames.size() < static_cast<size_t>(num_frames)) {
            int frame_idx = static_cast<int>(frames.size());
            frames.push_back(pag);
            ultimo_acesso.push_back(t);

            tabelas_paginas[id_proc][num_pag].presente = true;
            tabelas_paginas[id_proc][num_pag].frame_fisico = frame_idx;
        } else {
            // 6 & 7. RAM cheia: roda o algoritmo LRU para escolher página vítima
            int min_idx = 0;
            int min_t = ultimo_acesso[0];
            for (size_t i = 1; i < frames.size(); ++i) {
                if (ultimo_acesso[i] < min_t) {
                    min_t = ultimo_acesso[i];
                    min_idx = i;
                }
            }

            Pagina antiga = frames[min_idx];

            // Define presente = false e frame_fisico = -1 na tabela do dono da vítima
            tabelas_paginas[antiga.id_processo][antiga.numero_pagina].presente = false;
            tabelas_paginas[antiga.id_processo][antiga.numero_pagina].frame_fisico = -1;

            // Insere a nova página no frame do LRU e atualiza a sua tabela de páginas
            frames[min_idx] = pag;
            ultimo_acesso[min_idx] = t;

            tabelas_paginas[id_proc][num_pag].presente = true;
            tabelas_paginas[id_proc][num_pag].frame_fisico = min_idx;
        }
    }

    return page_faults;
}

int simular_otimo(int num_frames, int tamanho_pagina_mb, 
                  const std::vector<Processo>& processos, 
                  const std::vector<std::pair<int, int>>& linha_tempo) {
    
    std::vector<Pagina> referencias = gerar_referencias_pagina(processos, linha_tempo, tamanho_pagina_mb);
    if (num_frames <= 0 || referencias.empty()) return 0;

    // Inicializa a Tabela de Páginas de cada processo com o tamanho adequado (Demand Paging)
    std::unordered_map<int, std::vector<EntradaTabelaPagina>> tabelas_paginas;
    for (const auto& p : processos) {
        int num_paginas = (p.memoria_mb + tamanho_pagina_mb - 1) / tamanho_pagina_mb;
        if (num_paginas <= 0) num_paginas = 1;
        tabelas_paginas[p.id] = std::vector<EntradaTabelaPagina>(num_paginas);
    }

    std::vector<Pagina> frames;
    int page_faults = 0;

    for (int t = 0; t < static_cast<int>(referencias.size()); ++t) {
        const auto& pag = referencias[t];
        int id_proc = pag.id_processo;
        int num_pag = pag.numero_pagina;

        // 1 & 2. CPU pede a página e consulta a tabela de páginas
        if (tabelas_paginas[id_proc][num_pag].presente) {
            // 3. Hit! No Ótimo, não precisa fazer nada especial
            continue;
        }

        // 4. Se presente for false, ocorreu um Page Fault
        page_faults++;

        // 5. Se houver espaço livre na RAM, aloque a página em um frame vazio
        if (frames.size() < static_cast<size_t>(num_frames)) {
            int frame_idx = static_cast<int>(frames.size());
            frames.push_back(pag);

            tabelas_paginas[id_proc][num_pag].presente = true;
            tabelas_paginas[id_proc][num_pag].frame_fisico = frame_idx;
        } else {
            // 6 & 7. RAM cheia: roda o algoritmo Ótimo para escolher página vítima
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
                Pagina antiga = frames[idx_para_substituir];

                // Define presente = false e frame_fisico = -1 na tabela do dono da vítima
                tabelas_paginas[antiga.id_processo][antiga.numero_pagina].presente = false;
                tabelas_paginas[antiga.id_processo][antiga.numero_pagina].frame_fisico = -1;

                // Insere a nova página no frame e atualiza a sua tabela de páginas
                frames[idx_para_substituir] = pag;
                tabelas_paginas[id_proc][num_pag].presente = true;
                tabelas_paginas[id_proc][num_pag].frame_fisico = idx_para_substituir;
            }
        }
    }

    return page_faults;
}
