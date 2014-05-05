/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          attributeregister.inc
 *  Type:          Module include
 *  Description:   Manages a list of every attribute and map them to modules.
 *
 *  Copyright (C) 2009-2011  Greyscale, Richard Helgeby
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

/**
 * Attribute register data (adt trie).
 */
static Handle:ClassAttribRegData = INVALID_HANDLE;

/**
 * Creates the attribute register. This is only created once, during module
 * registration, and will not be destroyed until the plugin is unloaded.
 */
ClassAttribReg_Create()
{
    ClassAttribRegData = CreateTrie();
}


/***************************
 *    PUBLIC MODULE API    *
 ***************************/

/**
 * Adds a attribue module to the register.
 *
 * @param attribue  Attribute module.
 * @param name      Name of the attribute.
 *
 * @return          0 on success.
 *                  -1 if invalid module.
 *                  -2 if invalid name.
 *                  -3 if attribute already exist.
 */
stock ClassAttribReg_AddAttrib(Module:attribute, const String:name[])
{
    // Validate module.
    if (attribute == INVALID_MODULE)
    {
        // Invalid module.
        return -1;
    }
    
    // Validate name.
    if (strlen(name) == 0)
    {
        // Invalid name.
        return -2;
    }
    
    // Try to add.
    if (!SetTrieValue(ClassAttribRegData, name, attribute, false))
    {
        // Attribute already exist.
        return -3;
    }
    
    return 0;
}

/**
 * Removes an attribute from the register.
 *
 * @param name      Name of attribute to remove.
 *
 * @return          True if removed, false if it doesn't exist.
 */
stock bool:ClassAttribReg_RemoveAttrib(const String:name[])
{
    return RemoveFromTrie(ClassAttribRegData, name);
}

/**
 * Gets the attribute module mapped to the specified name.
 *
 * @param name      Name of attribute to get.
 *
 * @return          Attribute module on success, otherwise INVALID_MODULE.
 */
stock Module:ClassAttribReg_GetAttrib(const String:name[])
{
    new Module:attribute;
    if (GetTrieValue(ClassAttribRegData, name, attribute))
    {
        return attribute;
    }
    return INVALID_MODULE;
}
