// Notifications push : ntfy (+ webhook optionnel, ex. n8n).
// Envoi dans une tâche FreeRTOS dédiée pour ne pas bloquer la boucle.
#pragma once
#include <cstdint>

enum class NotifyPrio : uint8_t { Low = 2, Default = 3, High = 4, Urgent = 5 };

void notifyBegin();
// Copie le message dans une file ; retourne false si la file est pleine.
bool notify(const char *title, const char *body, NotifyPrio prio, const char *tags = "");
