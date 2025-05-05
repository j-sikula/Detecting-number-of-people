#ifndef PERSON_MOVEMENT_H
#define PERSON_MOVEMENT_H
#include <stdint.h>
#include <stdlib.h>

#define EXIT_BORDER_INDEX_CONDITION <= 3

typedef struct person_movement
{
    uint8_t start_position;   // index of the start position
    uint8_t started_exiting; // Flag to check if the person started exiting, default is false
    uint8_t current_position; // index of the current position
    uint8_t updated_position; // Flag to check if the position is updated, default is true
    uint8_t delete_movement;  // Flag to check if the movement should be deleted, default is false
    struct person_movement *next_element_pointer;
} person_movement_t;

typedef struct local_minimum_list
{
    uint8_t index;
    struct local_minimum_list *next_element_pointer;
} local_minimum_list_t;

void add_local_minimum(local_minimum_list_t **start_pointer, uint8_t index);

void remove_local_minimum(local_minimum_list_t **start_pointer, uint8_t index);

void clear_local_minimum_list(local_minimum_list_t **start_pointer);

person_movement_t *create_list(uint8_t start_position_index);

uint8_t add_to_list(person_movement_t **start_pointer, uint8_t start_position_index);

uint8_t remove_unactive_from_list(person_movement_t **start_pointer);

uint8_t remove_from_list(person_movement_t **start_pointer, person_movement_t *to_remove);

uint8_t clear_list(person_movement_t **start_pointer);

/// @brief updates current position and sets flags
/// @param pointer 
/// @param new_position 
void update_position(person_movement_t *pointer, uint8_t new_position);

/// @brief resets person movement as it was deleted and created once again
/// @param pointer 
void reset_person_movement(person_movement_t *pointer, uint8_t start_position_index);

uint8_t get_column(uint8_t index);

uint8_t is_current_index_in_list(person_movement_t *start_pointer, uint8_t index);


#endif

