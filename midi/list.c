#include <stdlib.h>
#include "midi.h"
#include "list.h"

struct MIDIListEntry;

struct MIDIListEntry {
  void * item;
  struct MIDIListEntry * next;
};

/**
 * @ingroup MIDI
 * @brief A list of MIDI objects of a common type.
 * I realize that this a general purpose linked list and should not be
 * restricted to MIDIKit use as the MIDI-prefix might indicate.
 * I will evaluate and extract general purpose code at a later point.
 */
struct MIDIList {
  int refs;
  void (*retain)( void * item );
  void (*release)( void * item );
  struct MIDIListEntry * data;
};

/**
 * @internal Methods for internal use.
 * @{
 */

/**
 * @brief Use the callback to retain a list item.
 * @private @memberof MIDIList
 * @param list The list.
 * @param item The item to retain.
 */
static void _list_item_retain( struct MIDIList * list, void * item ) {
  if( item != NULL && list->retain != NULL ) {
    (*list->retain)( item );
  }
}

/**
 * @brief Use the callback to release a list item.
 * @private @memberof MIDIList
 * @param list The list.
 * @param item The item to release.
 */
static void _list_item_release( struct MIDIList * list, void * item ) {
  if( item != NULL && list->release != NULL ) {
    (*list->release)( item );
  }
}

/** @} */

#pragma mark Creation and destruction
/**
 * @name Creation and destruction
 * Creating, destroying and reference counting of MIDIConnector objects.
 * @{
 */

/**
 * @brief Create a MIDIList instance.
 * Allocate space and initialize a MIDIList instance.
 * @public @memberof MIDIList
 * @param retain The callback to use for retaining list items.
 * @param retain The callback to use for releasing list items.
 * @return a pointer to the created list structure on success.
 * @return a @c NULL pointer if the list could not created.
 */
struct MIDIList * MIDIListCreate( void (*retain)( void * ), void (*release)( void * ) ) {
  struct MIDIList * list = malloc( sizeof( struct MIDIList ) );
  MIDIPrecondReturn( list != NULL, ENOMEM, NULL );

  list->refs = 1;
  list->retain  = retain;
  list->release = release;
  list->data = NULL;

  return list;
}

/**
 * @brief Destroy a MIDIList instance.
 * Free all resources occupied by the list and release all
 * list items.
 * @public @memberof MIDIList
 * @param list The list.
 */
void MIDIListDestroy( struct MIDIList * list ) {
  struct MIDIListEntry * entry;
  struct MIDIListEntry * next;
  MIDIPrecondReturn( list != NULL, EFAULT, (void)0 );
  entry = list->data;
  list->data = NULL;
  while( entry != NULL ) {
    next = entry->next;
    _list_item_release( list, entry->item );
    free( entry );
    entry = next;
  }
  free( list );
}

/**
 * @brief Retain a MIDIList instance.
 * Increment the reference counter of a list so that it won't be destroyed.
 * @public @memberof MIDIList
 * @param list The list.
 */
void MIDIListRetain( struct MIDIList * list ) {
  MIDIPrecondReturn( list != NULL, EFAULT, (void)0 );
  list->refs++;
}

/**
 * @brief Release a MIDIList instance.
 * Decrement the reference counter of a list. If the reference count
 * reached zero, destroy the list.
 * @public @memberof MIDIList
 * @param list The list.
 */
void MIDIListRelease( struct MIDIList * list ) {
  MIDIPrecondReturn( list != NULL, EFAULT, (void)0 );
  if( ! --list->refs ) {
    MIDIListDestroy( list );
  }
}

/** @} */

#pragma mark List management
/**
 * @name List management
 * Functions for working with lists.
 * @{
 */

/**
 * @brief Add an item to the list.
 * Add one item to the list and retain it.
 * @public @memberof MIDIList
 * @param list The list.
 * @param item The item to add.
 * @retval 0 on success.
 * @retval >0 otherwise.
 */
int MIDIListAdd( struct MIDIList * list, void * item ) {
  struct MIDIListEntry * entry;
  MIDIPrecond( list != NULL, EFAULT );
  MIDIPrecond( item != NULL, EINVAL );

  entry       = malloc( sizeof( struct MIDIListEntry ) );
  entry->item = item;
  entry->next = list->data;
  _list_item_retain( list, entry->item );
  list->data  = entry;
  return 0;
}

/**
 * @brief Remove an item from the list.
 * Remove one item from the list and release it.
 * @public @memberof MIDIList
 * @param list The list.
 * @param item The item to remove.
 * @retval 0 on success.
 * @retval >0 otherwise.
 */
int MIDIListRemove( struct MIDIList * list, void * item ) {
  struct MIDIListEntry * entry;
  struct MIDIListEntry ** eptr;
  MIDIPrecond( list != NULL, EFAULT );
  MIDIPrecond( item != NULL, EINVAL );

  eptr  = &(list->data);
  entry = list->data;
  while( entry != NULL ) {
    if( entry->item == item ) {
      _list_item_release( list, entry->item );
      *eptr = entry->next;
      free( entry );
      entry = *eptr;
    } else {
      eptr  = &(entry->next);
      entry = entry->next;
    }
  }
  return 0;
}


/**
 * @brief Apply a function to all items in the list.
 * Call the given function once for every item in the list.
 * Use the given @c info pointer as the first parameter when
 * calling the function and the item as the second parameter.
 * @public @memberof MIDIList
 * @param list The list.
 * @param func The function to apply.
 * @param info The pointer to pass as the first parameter.
 * @retval 0 on success.
 * @retval >0 otherwise.
 */
int MIDIListApply( struct MIDIList * list, void * info, int (*func)( void *, void * ) ) {
  struct MIDIListEntry * entry;
  void * item;
  int result = 0;
  MIDIPrecond( list != NULL, EFAULT );
  MIDIPrecond( func != NULL, EINVAL );

  entry = list->data;
  while( entry != NULL ) {
    item  = entry->item;
    entry = entry->next;
    if( item != NULL ) {
      result += (*func)( item, info );
    }
  }
  return result;
}

/** @} */