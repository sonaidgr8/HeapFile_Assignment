#include "DiskFile.h"
#include "Page.h"

       /* Given a reference (pointer to pointer) to the head
       of a list and an int, appends a new node at the end  */
        void DiskFile :: appendPages(struct Node** head_ref, Page new_data)
        {
            struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
            struct Node *last = *head_ref;
            new_node->data  = new_data;
            new_node->next = NULL;
            if (*head_ref == NULL)
            {
               new_node->prev = NULL;
               *head_ref = new_node;
               return;
            }
            while (last->next != NULL)
                last = last->next;
            last->next = new_node;
            new_node->prev = last;
            return;
        }

       /* Given a reference (pointer to pointer) to the head
       of a list and a record details, it inserts the record in the DiskFIle  */
        void DiskFile :: insertRecord(struct Node** head_ref, int rec_id, int rec_length, bool * inserted_records)
        {
            if(rec_length > DISK_PAGE_SIZE){
                printf("Record length should be less than maximum allowable Page size %d\n", DISK_PAGE_SIZE);
                *inserted_records = false;
                return;
            }
            else if(this->totalPages * DISK_PAGE_SIZE > DISK_FILE_SIZE){
                printf("Number of Pages must not exhaust maximum allowable DiskFile size %d\n", DISK_FILE_SIZE);
                *inserted_records = false;
                return;
            }
            struct Node *last = *head_ref;
            int count = 0;
            while(last != NULL) {
                count = count+1;
                /* Case-1: When DataPages are empty at the beginning */
                if (((last->data.dirSlotCount == 1) && (last->data.arr[0].valid == false)) || (last->data.spaceLeft == DISK_PAGE_SIZE)){
                    last->data.arr[0].id = rec_id;
                    last->data.arr[0].length = rec_length;
                    last->data.arr[0].start = 0;
                    last->data.arr[0].valid = true;
                    last->data.spaceLeft = last->data.spaceLeft - (rec_length + DIR_ENTRY_LENGTH);
                    last->data.dirSlotCount = last->data.arr.size();
                    printf("Record inserted in Page : %d \n", count);
                    *inserted_records = true;
                    break;
                }
                else{
                /* Case-2: When an empty slot is available from deletion to hold the Record */
                    for(int i=0; i<last->data.arr.size(); i++){
                        if(last->data.arr[i].id == 0 && last->data.arr[i].valid == false){
                            bool space_available = (rec_length <= (last->data.arr[i].start + last->data.arr[i].length));
                            if(space_available){
                                last->data.arr[i].id = rec_id;
                                last->data.arr[i].length = rec_length;
                                last->data.arr[i].valid = true;
                                printf("Record inserted in an empty slot of Page : %d \n", count);
                                return;
                            }
                        }
                    }
                /* Case-3: When no empty slot is available and if existing Page can accommodate the data at the end */
                    if (last->data.spaceLeft - (rec_length + DIR_ENTRY_LENGTH) > 0){
                        int next_start_idx = last->data.arr[last->data.arr.size()-1].start + last->data.arr[last->data.arr.size()-1].length;
                        last->data.arr.push_back(DirectoryEntry(rec_id, rec_length, next_start_idx, true));
                        last->data.spaceLeft = last->data.spaceLeft - (rec_length + DIR_ENTRY_LENGTH);
                        last->data.dirSlotCount = last->data.arr.size();
                        printf("Record inserted in Page : %d \n", count);
                        return;
                    }
                    else{
                /* Case-4: When no empty slot is available and if existing Page can not accommodate the data at the end,
                 then go to next page to check availability */
                        if(count<this->totalPages){
                            printf("Exhausted space in Page %d. Moving on to Page %d to insert Record...\n", count, count+1);
                        }
                 /* Case-5: When no empty slot is available and no existing Page can accommodate the data,
                 then append a new page if maximum allowable DiskFile size is not exhausted */
                        else{
                            if((this->totalPages + 1) * DISK_PAGE_SIZE <=  DISK_FILE_SIZE){
                                printf("Exhausted space in Page %d. Page %d is created to insert this new Record!\n", count, count+1);
                                appendPages(&this->nodePointer, Page());
                                this->totalPages = this->totalPages + 1;
                            }
                            else{
                                printf("Exhausted maximum allowable DiskFile size %d.\n", DISK_FILE_SIZE);
                                return;
                            }
                        }
                      last = last->next;
                    }
                }
            }
          return;
        }

       /* Given a reference pointer to the head
       of a list and an Record ID, it deletes the corresponding record in a Page  */
        void DiskFile :: deleteRecord(struct Node** head_ref, int rec_id)
        {
            struct Node *last = *head_ref;
            bool flag = false; /* To check for duplicate entries. Currently deletes all
            duplicate entries by traversing all the pages. */
            int count = 0;
            while(last != NULL) {
                count = count + 1;
                for(int i=0; i<last->data.arr.size(); i++){
                    if(last->data.arr[i].id == rec_id && last->data.arr[i].valid == true){
                        last->data.arr[i].id = 0;
                        int gap = (i == last->data.arr.size()-1) ? 0 : last->data.arr[i+1].start - (last->data.arr[i].start + last->data.arr[i].length);
                        /* Claim the left-over space through gap, restores original slot's length but not defined for last Record of a Page */
                        last->data.arr[i].length = last->data.arr[i].length + gap;
                        last->data.arr[i].valid = false;
                        flag = true;
                        printf("Record deleted from Page : %d \n", count);
                        /* No return after one deletion. After one entry deletion it checks
                        for all the duplicate entries matching IDs and deletes them. */
                    }
                }
              last = last->next;
            }

          if(flag==false) // If not a single deletion has happened
            printf("Sorry the entered Record ID does not exist in any of the Pages...\n");

          return;
        }

        /* This function prints contents of DiskFIle starting in terms of Pages & Records */
        void DiskFile :: printPages(struct Node* node)
        {
            struct Node* last;
            printf("DiskFile traversal, displaying Pages with Records <ID,Start,Length-Gap,Valid> :\n");
            int count = 0;
            while (node != NULL) {
                count = count+1;
                printf("Page:%d  Records{", count);
                for(int i=0; i<node->data.arr.size(); i++){
                    int gap = (i == node->data.arr.size()-1) ? 0 : node->data.arr[i+1].start - (node->data.arr[i].start + node->data.arr[i].length);
                    if(i!=node->data.arr.size()-1)
                        printf("<%d,%d,%d-%d,%s> ", node->data.arr[i].id, node->data.arr[i].start, node->data.arr[i].length, gap, node->data.arr[i].valid ? "true" : "false");
                    else
                        printf("<%d,%d,%d-%d,%s>", node->data.arr[i].id, node->data.arr[i].start, node->data.arr[i].length, gap, node->data.arr[i].valid ? "true" : "false");
                }
                printf("}\n");
                last = node;
                node = node->next;
            }
        }
