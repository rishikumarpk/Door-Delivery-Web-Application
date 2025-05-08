#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Product {
    char name[64];
    char image[256];
    int price;
    int stock;
    struct Product *next;
} Product;

typedef struct CartEntry {
    char name[64];
    int quantity;
    int price;
    struct CartEntry *next;
} CartEntry;

// Load products from all category files
Product* load_all_products() {
    const char *categories[] = {"fruits", "vegetables", "snacks", "dairy", "cleaning", "beverages"};
    int num_categories = sizeof(categories) / sizeof(categories[0]);
    
    Product *head = NULL;
    
    for (int i = 0; i < num_categories; i++) {
        char path[512];
        snprintf(path, sizeof(path), "C:\\Apache24\\data\\%s.txt", categories[i]);
        
        FILE *fp = fopen(path, "r");
        if (!fp) {
            fprintf(stderr, "Failed to open %s\n", path);
            continue;
        }

        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = 0;
            Product p = {0};
            if (sscanf(line, "%63[^,],%255[^,],%d,%d", p.name, p.image, &p.price, &p.stock) == 4) {
                Product *node = malloc(sizeof(Product));
                *node = p;
                node->next = head;
                head = node;
            }
        }
        fclose(fp);
    }
    
    return head;
}

// Check if an item already exists in the cart entries
CartEntry* find_cart_entry(CartEntry *head, const char *name) {
    CartEntry *current = head;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Load the cart from cart.txt with consolidated entries
void load_cart_and_print(Product *product_list, const char *cart_path) {
    FILE *fp = fopen(cart_path, "r");
    if (!fp) {
        printf("Content-type: text/plain\r\n\r\nCart is empty.\n");
        return;
    }

    // Create a linked list to consolidate cart entries
    CartEntry *cart_head = NULL;
    
    // Read the cart file and consolidate quantities
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        char item_name[64];
        int qty;

        if (sscanf(line, "%63[^,],%d", item_name, &qty) == 2) {
            // Find the product price
            Product *p = product_list;
            int price = 0;
            
            while (p) {
                if (strcmp(p->name, item_name) == 0) {
                    price = p->price;
                    break;
                }
                p = p->next;
            }
            
            if (price > 0) {
                // Check if item already exists in our consolidated list
                CartEntry *existing = find_cart_entry(cart_head, item_name);
                
                if (existing) {
                    // Update quantity for existing entry
                    existing->quantity += qty;
                } else {
                    // Create new entry
                    CartEntry *new_entry = malloc(sizeof(CartEntry));
                    strncpy(new_entry->name, item_name, sizeof(new_entry->name) - 1);
                    new_entry->name[sizeof(new_entry->name) - 1] = '\0';
                    new_entry->quantity = qty;
                    new_entry->price = price;
                    new_entry->next = cart_head;
                    cart_head = new_entry;
                }
            }
        }
    }
    fclose(fp);
    
    // Print CSV headers
    printf("Content-type: text/plain\r\n\r\n");
    printf("Item,Quantity,Unit Price,Line Total\n");
    
    // Print consolidated cart entries
    CartEntry *current = cart_head;
    while (current) {
        int line_total = current->price * current->quantity;
        printf("%s,%d,%d,%d\n", current->name, current->quantity, current->price, line_total);
        current = current->next;
    }
    
    // Clean up cart entries
    current = cart_head;
    while (current) {
        CartEntry *temp = current;
        current = current->next;
        free(temp);
    }
}

int main(void) {
    const char *cart_file = "C:\\Apache24\\data\\cart.txt";

    // Load product data from all category files
    Product *product_list = load_all_products();
    if (!product_list) {
        printf("Content-type: text/plain\r\n\r\nFailed to load products.\n");
        return 1;
    }

    // Load cart and display the cart details
    load_cart_and_print(product_list, cart_file);

    // Clean up the product list
    Product *current = product_list, *temp;
    while (current) {
        temp = current->next;
        free(current);
        current = temp;
    }

    return 0;
}