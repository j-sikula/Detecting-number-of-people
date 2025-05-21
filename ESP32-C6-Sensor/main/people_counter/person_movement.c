#include "person_movement.h"

// https://www.geeksforgeeks.org/linked-list-in-c/
// https://www.learn-c.org/en/Linked_lists

void add_local_minimum(local_minimum_list_t **start_pointer, uint8_t index)
{
    if (*start_pointer == NULL)
    {
        *start_pointer = malloc(sizeof(local_minimum_list_t)); // allocate memory for the first element
        if (*start_pointer != NULL)
        {
            (*start_pointer)->index = index; // set the index of the first element
            (*start_pointer)->next_element_pointer = NULL; // set the next element pointer to NULL
        }
    }
    else
    {
        local_minimum_list_t *list_element = *start_pointer;
        while (list_element->next_element_pointer != NULL) // iterate through list to the end
        {
            list_element = list_element->next_element_pointer;
        }
        local_minimum_list_t *new_element = malloc(sizeof(local_minimum_list_t)); // allocate memory for the new element
        if (new_element != NULL)
        {
            new_element->index = index; // set the index of the new element
            new_element->next_element_pointer = NULL; // set the next element pointer to NULL
            list_element->next_element_pointer = new_element; // link the new element to the end of the list
        }
    }
}

void remove_local_minimum(local_minimum_list_t **start_pointer, uint8_t index)
{
    if ((*start_pointer)->index == index) // check if the first element is the one to be removed
    {
        local_minimum_list_t *temp = *start_pointer; // store the head pointer in a temporary variable
        *start_pointer = (*start_pointer)->next_element_pointer; // update the head pointer to the next element
        free(temp); // free the memory of the removed element
    }
    else
    {
        local_minimum_list_t *list_element = *start_pointer;
        while (list_element->next_element_pointer != NULL && list_element->next_element_pointer->index != index) // iterate through list to find the element to remove
        {
            list_element = list_element->next_element_pointer;
        }
        if (list_element->next_element_pointer != NULL) // check if the element was found
        {
            local_minimum_list_t *temp = list_element->next_element_pointer; // store the element to be removed in a temporary variable
            list_element->next_element_pointer = temp->next_element_pointer; // link the previous element to the next element of the one being removed
            free(temp); // free the memory of the removed element
        }
    }
}

void clear_local_minimum_list(local_minimum_list_t **start_pointer)
{
    while (*start_pointer != NULL) // iterate through the list
    {
        local_minimum_list_t *temp = *start_pointer; // store the head pointer in a temporary variable
        *start_pointer = (*start_pointer)->next_element_pointer; // update the head pointer to the next element
        free(temp); // free the memory of the removed element
    }
    
}

person_movement_t *create_list(uint8_t start_position_index)
{
    person_movement_t *list_element = malloc(sizeof(person_movement_t)); // allocate memory for the first element
    if (list_element != NULL)
    {
        list_element->start_position = start_position_index;   // set the start position
        list_element->started_exiting = start_position_index % 8 EXIT_BORDER_INDEX_CONDITION;  // set the started exiting flag based on the column index
        list_element->current_position = start_position_index; // set the current position
        list_element->updated_position = 1;                    // set the updated position flag to true
        list_element->delete_movement = 0;                     // set the delete movement flag to false
        list_element->next_element_pointer = NULL;             // set the next element pointer to NULL
        return list_element;
    }
    else
    {
        return NULL; // return NULL if memory allocation fails
    }
}

uint8_t add_to_list(person_movement_t **start_pointer, uint8_t start_position_index)
{
    person_movement_t *list_element = *start_pointer;
    if (list_element == NULL)
    {
        *start_pointer = create_list(start_position_index); // create new element
        return 1;
    }
    uint8_t size = 2; // first
    while (list_element->next_element_pointer != 0)
    {
        list_element = list_element->next_element_pointer; // iterate through list to the end
        size++;
    }
    person_movement_t *pointer_to_new_element = create_list(start_position_index); // create new element
    if (pointer_to_new_element == NULL)
    {
        // ESP_LOGE(TAG, "Memory allocation failed for new element.");
        return --size;
    }
    list_element->next_element_pointer = pointer_to_new_element;
    return size;
}

uint8_t remove_unactive_from_list(person_movement_t **start_pointer)
{
    person_movement_t *list_element = *start_pointer;
    while (list_element != NULL) // iterate through the list
    {
        

        if (list_element->delete_movement == 1) // check if the element should be deleted
        {
            person_movement_t *temp = list_element;            // store the current element in a temporary variable
            list_element = list_element->next_element_pointer; // move to the next element
            remove_from_list(start_pointer, temp);             // remove the current element from the list
        }
        else
        {
            if (list_element->updated_position == 1) // check if the position is updated
            {
                list_element->updated_position = 0; // reset the updated position flag
            }
            else
            {
                list_element->delete_movement = 1; // set the delete movement flag to true
            }
            list_element = list_element->next_element_pointer; // move to the next element
        }
    }
    return 0;
}

uint8_t remove_from_list(person_movement_t **start_pointer, person_movement_t *to_remove)
{
    if (*start_pointer == to_remove)
    {
        person_movement_t *temp = *start_pointer;                // store the head pointer in a temporary variable
        *start_pointer = (*start_pointer)->next_element_pointer; // update the head pointer to the next element
        free(temp);                                              // free the memory of the removed element
        return 0;
    }

    person_movement_t *list_element = *start_pointer;
    while (list_element->next_element_pointer != to_remove && list_element->next_element_pointer != NULL)
    {
        list_element = list_element->next_element_pointer; // iterate through list to the end
    }
    if (list_element->next_element_pointer == NULL)
    {
        // ESP_LOGE(TAG, "Element not found in the list.");
        return 1; // element not found
    }
    list_element->next_element_pointer = to_remove->next_element_pointer; // remove element from list
    free(to_remove);                                                      // free memory of the removed element
    return 0;
}

uint8_t clear_list(person_movement_t **start_pointer)
{
   
    while (*start_pointer != NULL) // iterate through the list
    {
        remove_from_list(start_pointer, *start_pointer);             // remove the current element from the list
    }
    return 0;
}

void update_position(person_movement_t *pointer, uint8_t new_position)
{
    if (pointer != NULL)
    {
        pointer->current_position = new_position; // update the current position of the element
        pointer->updated_position = 1;            // set the updated position flag to true
        pointer->delete_movement = 0;             // set the delete movement flag to false
    }
    else
    {
        // ESP_LOGE(TAG, "Pointer is NULL, cannot update position.");
    }
}

void reset_person_movement(person_movement_t *pointer, uint8_t start_position_index)
{
    if (pointer != NULL)
    {
        pointer->start_position = start_position_index;   // reset the start position
        pointer->started_exiting = start_position_index % 8 EXIT_BORDER_INDEX_CONDITION;  // reset the started exiting flag based on the column index
        pointer->current_position = start_position_index; // reset the current position
        pointer->updated_position = 1;                    // set the updated position flag to true
        pointer->delete_movement = 0;                     // set the delete movement flag to false
    }
    else
    {
        // ESP_LOGE(TAG, "Pointer is NULL, cannot reset person movement.");
    }
}

uint8_t get_column(uint8_t index)
{
    return index % 8;
}

uint8_t is_current_index_in_list(person_movement_t *start_pointer, uint8_t index)
{
    person_movement_t *list_element = start_pointer;
    while (list_element != NULL) // iterate through the list
    {
        if (list_element->current_position == index) // check if the current position matches the index
        {
            return 1; // index found in the list
        }
        list_element = list_element->next_element_pointer; // move to the next element
    }
    return 0;
}
