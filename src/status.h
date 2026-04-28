/**
 * @file status.h
 * @brief Status summary command for the vibe-req CLI.
 *
 * Provides a command-level renderer that counts discovered entities by
 * lifecycle status, lifecycle priority, and entity kind.
 */

#ifndef VIBE_STATUS_H
#define VIBE_STATUS_H

#include "entity.h"

/**
 * Print entity counts grouped by status, priority, and kind.
 *
 * @param elist  full entity list to inspect
 */
void cmd_status(const EntityList *elist);

#endif /* VIBE_STATUS_H */
