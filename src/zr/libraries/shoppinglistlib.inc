/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          shoppinglistlib.inc
 *  Type:          Library
 *  Description:   Formats raw strings into shopping list-styled lists.
 *
 *  Copyright (C) 2009-2011  Greyscale
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================================
 */

#if defined _shoppinglistlib_included
 #endinput
#endif
#define _shoppinglistlib_included

/**
 * This file is meant for formatting a given string into a list formatted around the style of a shopping
 * list.
 * Example:
 * String: "Bacon, Egg, Egg, Milk, Butter"
 * Shopping List: "Bacon, Eggx2, Milk, Butter"
 * Function parameters will include options to change the "," to any character.  As well as the "x2" format.
 * 
 * More complex examples:
 * String: "Red - Green - Blue-Red- Green -Green - Yellow -Grey"  Note: Horribly formatted, but we can handle it.
 * Shopping List: "Red(2)\nGreen(3)\nBlue\nYellow\nGrey" Note: We can change what to separate each item with, as well as how to display the quantity.
 * 
 * List Options:
 * 
 * Construct: Take a string and dump out a list.  Simple.
 * Count: Count how many items are in the raw list.
 * Add: Add an item to a raw list. 
 * Remove: Removes an item from the raw list.  Must remove from the end to preserve list order.
 *  
 * Scenario:
 * String: "Apple, Orange, Mango, Orange"
 * Shopping List: "Apple, Orange x2, Mango"
 * Remove: "Orange"
 * Remove first orange: "Apple, Mango, Orange"  Different order.
 * Remove last orange: "Apple, Orange, Mango" Yay.
 * 
 */

/**
 * @section String length defines.
 */
#define SHOPPINGLIST_MAXITEMS 32
#define SHOPPINGLIST_MAXLEN   64
/**
 * @endsection
 */

/**
 * Takes a given string and outputs a re-formatted shopping list styled list.  See description above.
 * Note:  This is for display purposes only, meaning there will be no API made to format the output of this function.
 * If you plan on the list being dynamic, store the raw string and use the API to edit those.
 * 
 * @param list              Raw string to be formatted.
 * @param shoppinglist      Outputted shopping list.
 * @param slist_maxlen      The maximum length of the shopping list.
 * @param in_token          The token used to separate each item in the raw string. Ex: "Apple, Orange"  Token: ","
 * @param out_token         What to separate each item with in the shopping list.  Ex: Token: "\n"  List: "Apple\nOrange"
 * @param quantityformat    How to show the quantity of an item for multi-listed items.  ## is the number.  Ex: "Apples x2" quantityformat: " x##"
 */
stock ShopListLib_Construct(const String:list[], String:shoppinglist[], slist_maxlen, const String:in_token[] = ",", const String:out_token[] = ",", const String:quantityformat[] = " x##")
{
    // Put each item in a list.
    new String:items[SHOPPINGLIST_MAXITEMS][SHOPPINGLIST_MAXLEN];
    ExplodeString(list, in_token, items, SHOPPINGLIST_MAXITEMS, SHOPPINGLIST_MAXLEN);
    
    // Put each item in a trie where we count each item.
    new Handle:trieItems = CreateTrie();
    new item_count;
    new validstrings;
    
    for (new itemindex = 0; itemindex < SHOPPINGLIST_MAXITEMS; itemindex++)
    {
        // Trim off any whitespace.
        TrimString(items[itemindex]);
        
        // Check if there is an item in this index.
        if (items[itemindex][0] != 0)
        {
            // Reset item_count to 0, retrieve value, if it already exists, and set value + 1.
            item_count = 0;
            GetTrieValue(trieItems, items[itemindex], item_count);
            SetTrieValue(trieItems, items[itemindex], ++item_count);
            
            validstrings++;
        }
    }
    
    // Initialize 'shoppinglist'.
    strcopy(shoppinglist, slist_maxlen, "");
    
    decl String:item_formatted[SHOPPINGLIST_MAXLEN];
    decl String:strQuantity[8];
    
    for (new itemindex = 0; itemindex < validstrings; itemindex++)
    {
        // Get quantity count, in string and int.
        GetTrieValue(trieItems, items[itemindex], item_count);
        
        // If the item_count is 0, this item is already in the shopping list.
        if (item_count == 0)
            continue;
        
        strcopy(item_formatted, sizeof(item_formatted), items[itemindex]);
        
        // Format the quantity on if there was more than 1.
        if (item_count > 1)
        {
            IntToString(item_count, strQuantity, sizeof(strQuantity));
            Format(item_formatted, sizeof(item_formatted), "%s%s", item_formatted, quantityformat);
            ReplaceString(item_formatted, sizeof(item_formatted), "##", strQuantity);
        }
        
        // Format the item onto the list.
        // If the list is empty, copy, not append.
        if (shoppinglist[0] == 0)
            strcopy(shoppinglist, slist_maxlen, item_formatted);
        // If the list isn't empty, append, not copy.
        else
            Format(shoppinglist, slist_maxlen, "%s%s%s", shoppinglist, out_token, item_formatted);
        
        SetTrieValue(trieItems, items[itemindex], 0);
    }
    
    // Destroy the trie handle.
    CloseHandle(trieItems);
}

/**
 * Count how many of a certain item is in a list.
 * 
 * @param list          The raw list to remove item from.
 * @param item          The item to count. ("" to count all items)
 * @param token         The token used to separate each item in the raw string. Ex: "Apple, Orange"  Token: ","
 *  
 * @return              The number of items specified that are in the list.
 */
stock ShopListLib_Count(const String:list[], const String:item[], const String:token[])
{
    // Put each item in a list.
    new String:items[SHOPPINGLIST_MAXITEMS][SHOPPINGLIST_MAXLEN];
    ExplodeString(list, token, items, SHOPPINGLIST_MAXITEMS, SHOPPINGLIST_MAXLEN);
    
    new validstrings;
    for (new itemindex = 0; itemindex < SHOPPINGLIST_MAXITEMS; itemindex++)
    {
        // Trim off any whitespace.
        TrimString(items[itemindex]);
        
        // If there are no more items left, exit the loop.
        if (items[itemindex][0] == 0)
            break;

        if (item[0] != 0 && !StrEqual(item, items[itemindex], false))
            continue;

        validstrings++;
    }
    
    return validstrings;
}

/**
 * Add an item to the list.
 * Notes:
 * For max number of items, and the max length of each item, see SHOPPINGLIST_* defines at the top.
 * 
 * @param list          The raw list to add item to.
 * @param maxlen        The maximum length of the finished list.
 * @param item          The item to add from to list.
 * @param token         The token to add with the item. 
 */
public ShopListLib_Add(String:list[], maxlen, const String:item[], const String:token[])
{
    // If the list is empty, then just copy the item into it and return.
    if (list[0] == 0)
        strcopy(list, maxlen, item);
    
    // Format on the item.
    else
        Format(list, maxlen, "%s%s%s", list, token, item);
}

/**
 * Remove an item from the list.
 * Notes:
 * For max number of items, and the max length of each item, see SHOPPINGLIST_* defines at the top.
 * 
 * @param list          The raw list to remove item from.
 * @param maxlen        The maximum length of the finished list.
 * @param item          The item to remove from the list.
 * @param token         The token to remove with the item.
 *  
 * @return              True if the item was found and removed, false if not found. 
 */
public bool:ShopListLib_Remove(String:list[], maxlen, const String:item[], const String:token[])
{
    // Explode the items into an array.
    new String:items[SHOPPINGLIST_MAXITEMS][SHOPPINGLIST_MAXLEN];
    ExplodeString(list, token, items, SHOPPINGLIST_MAXITEMS, SHOPPINGLIST_MAXLEN);
    
    new validstrings;
    
    for (new itemindex = SHOPPINGLIST_MAXITEMS - 1; itemindex >= 0; itemindex--)
    {
        // Trim off any whitespace.
        TrimString(items[itemindex]);
        
        if (StrEqual(item, items[itemindex], false))
        {
            // Nullify the matching string.
            items[itemindex][0] = 0;
            
            // Re-copy each index after the matching string to overwrite it. 
            for (new newitemindex = itemindex; newitemindex < SHOPPINGLIST_MAXITEMS - 1; newitemindex++)
                items[newitemindex] = items[newitemindex + 1];
            
            // If this isn't the last item removed, then implode the rest together.
            if (validstrings + itemindex > 0)
            {
                // Implode the array of strings back together into a full string.
                ImplodeStrings(items, validstrings + itemindex, token, list, maxlen);
            }
            // If this is the last item, then just nullify the output.
            else
                list[0] = 0;
            
            return true;
        }
        
        // If this is a valid string, count it.
        if (items[itemindex][0] != 0)
            validstrings++;
    }
    
    return false;
}

// **********************************************
//                 BROKEN STUFF
//      Attempted versions of the above code.
// The working version is mostly (99%) Frus's code.
// **********************************************

/**
 * Add an item to the list.
 * 
 * @param list          The list to remove item from.
 * @param maxlen        The maximum length of the finished list.
 * @param item          The item to remove from the list.
 * @param token         The token to remove with the item. 
 * @return              True if the item was found and removed, false if not found. 
 */
/*stock bool:ShoppingListRemove(String:list[], maxlen, const String:item[], const String:token[])
{
    new itemindex = -1;
    new itemindex2 = -1;
    new pos;
    while (itemindex == itemindex2)
    {
        itemindex2 = pos = StrContains(list[pos], item, false);
        
        if (itemindex2 != -1)
        {
            itemindex = itemindex2;
            pos += 1;
            
            PrintToServer("%d %d %d %s", itemindex, itemindex2, pos, list[pos]);
        }
        else
            break;
    }
    
    if (itemindex == -1)
        return false;
    
    PrintToServer("position %d", itemindex);
    
    return true;
}*/

/**
 * Add an item to the list.
 * 
 * @param list          The list to remove item from.
 * @param maxlen        The maximum length of the finished list.
 * @param maxitems      The maximum number of shopping items.
 * @param maxitemlen    The maximum length of EACH item in the string.
 * @param item          The item to remove from the list.
 * @param token         The token to remove with the item. 
 * @return              True if the item was found and removed, false if not found. 
 */
/*stock bool:ShoppingListRemove(String:list[], maxlen, maxitems, maxitemlen, const String:item[], const String:token[])
{
    // Default initialized value = false.
    new bool:success;
    
    // Put each item in a list.
    new String:items[maxitems][maxitemlen];
    ExplodeString(list, token, items, maxitems, maxitemlen);
    
    // x = List index.
    for (new x = maxitems - 1; x >= 0; x--)
    {
        PrintToServer("%d %s", x, items[x]);
        if (StrEqual(items[x], item, false))
        {
            strcopy(items[x], maxitemlen, "");
            success = true;
            break;
        }
    }
    
    new String:items_after[maxitems][maxitemlen];
    new count;
    
    // x = List index.
    for (new x = 0; x < maxitems; x++)
    {
        PrintToServer("%d \"%s\"", x, items[x]);
        
        if (!items[x][0])
            continue;
        
        // Move valid item to new array, and increment count after.
        strcopy(items_after[count++], maxitemlen, items[x]);
    }
    
    // Join the valid strings back together.
    ImplodeStrings(items_after, count, token, list, maxlen);
    
    return success;
}*/
