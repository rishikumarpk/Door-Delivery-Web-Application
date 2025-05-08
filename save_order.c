#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ORDER_FILE "C:\\Apache24\\data\\orders.json"
#define MAX_POST_SIZE 100000
#define DEBUG_LOG "C:\\Apache24\\logs\\debug.log"

// Node for queue
typedef struct Node {
    char *order_json;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

void enqueue(Queue *q, const char *json) {
    Node *newNode = malloc(sizeof(Node));
    newNode->order_json = strdup(json);
    newNode->next = NULL;
    if (q->rear)
        q->rear->next = newNode;
    else
        q->front = newNode;
    q->rear = newNode;
}

void write_debug(const char *msg) {
    FILE *f = fopen(DEBUG_LOG, "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

char *read_post_data() {
    char *len_str = getenv("CONTENT_LENGTH");
    if (!len_str) return NULL;
    int len = atoi(len_str);
    if (len <= 0 || len > MAX_POST_SIZE) return NULL;

    char *data = malloc(len + 1);
    fread(data, 1, len, stdin);
    data[len] = '\0';
    return data;
}

int is_valid_json(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return *s == '{';
}

void load_existing_orders(Queue *q) {
    FILE *f = fopen(ORDER_FILE, "r");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return;
    }
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    // Simple JSON array parsing
    char *p = buffer;
    while (*p) {
        while (*p && (*p != '{')) p++;
        if (!*p) break;

        char *start = p;
        int braces = 0;
        do {
            if (*p == '{') braces++;
            else if (*p == '}') braces--;
            p++;
        } while (braces > 0 && *p);

        if (braces == 0) {
            char temp[2048];
            int len = p - start;
            strncpy(temp, start, len);
            temp[len] = '\0';
            enqueue(q, temp);
        }
    }

    free(buffer);
}

void write_orders_to_file(Queue *q) {
    FILE *f = fopen(ORDER_FILE, "w");
    if (!f) {
        write_debug("Failed to open order file for writing.");
        return;
    }

    fputc('[', f);
    Node *cur = q->front;
    while (cur) {
        fputs(cur->order_json, f);
        if (cur->next) fputc(',', f);
        cur = cur->next;
    }
    fputc(']', f);
    fclose(f);
}

void clear_cart() {
    FILE *f = fopen("C:\\Apache24\\data\\cart.txt", "w");
    if (f) fclose(f);
}

void free_queue(Queue *q) {
    Node *cur = q->front;
    while (cur) {
        Node *next = cur->next;
        free(cur->order_json);
        free(cur);
        cur = next;
    }
}

int main() {
    printf("Content-Type: application/json\r\n\r\n");

    char *post_data = read_post_data();
    if (!post_data || !is_valid_json(post_data)) {
        printf("{\"success\":false,\"message\":\"Invalid or missing order data\"}");
        return 1;
    }

    Queue q = {0};
    load_existing_orders(&q);
    enqueue(&q, post_data);

    write_orders_to_file(&q);
    clear_cart();
    write_debug("New order appended to queue and written to file.");

    printf("{\"success\":true}");

    free(post_data);
    free_queue(&q);
    return 0;
}
