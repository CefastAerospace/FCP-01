#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>

// ============================================================================
//  Módulo de Interface Web — WiFi + HTML Dashboard
//  >>> CAMADA TROCÁVEL <<<
//  No futuro, substitua este módulo por lora_interface.h/.cpp
//  sem alterar nenhum outro arquivo do projeto.
// ============================================================================

// Inicializa WiFi (conecta ao AP ou cria rede própria) e registra rotas HTTP
void web_init();

// Processa requisições HTTP (chamar no loop)
void web_loop();

#endif // WEB_INTERFACE_H
