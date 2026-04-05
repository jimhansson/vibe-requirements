#ifndef VIBE_REQUIREMENT_H
#define VIBE_REQUIREMENT_H

#define REQ_ID_LEN        64
#define REQ_TITLE_LEN    256
#define REQ_TYPE_LEN      32
#define REQ_STATUS_LEN    32
#define REQ_PRIORITY_LEN  32
#define REQ_DESC_LEN    1024
#define REQ_PATH_LEN     512

typedef struct {
    char id[REQ_ID_LEN];
    char title[REQ_TITLE_LEN];
    char type[REQ_TYPE_LEN];
    char status[REQ_STATUS_LEN];
    char priority[REQ_PRIORITY_LEN];
    char description[REQ_DESC_LEN];
    char file_path[REQ_PATH_LEN];
} Requirement;

typedef struct {
    Requirement *items;
    int          count;
    int          capacity;
} RequirementList;

void req_list_init(RequirementList *list);
int  req_list_add(RequirementList *list, const Requirement *req);
void req_list_free(RequirementList *list);

#endif /* VIBE_REQUIREMENT_H */
