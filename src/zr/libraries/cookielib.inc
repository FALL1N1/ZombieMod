/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          cookielib.inc
 *  Type:          Library
 *  Description:   Extended client cookie API.
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

#if defined _cookielib_included
 #endinput
#endif
#define _cookielib_included

#include <clientprefs>

/**
 * Check if a cookie has been initialized for a client.
 * Use a function below (or SetClientCookie) to initialize.
 * 
 * @param client    The client index.
 * @param cookie    The cookie handle.
 * 
 * @return          True if the cookie has an actual value, false if it's empty.
 */
stock bool:CookieLib_IsInitialized(client, Handle:cookie)
{
    // Get cookie string.
    decl String:strValue[8];
    GetClientCookie(client, cookie, strValue, sizeof(strValue));
    
    return (strValue[0]);
}

/**
 * Returns a boolean client cookie value.
 * 
 * @param client    The client index.
 * @param cookie    The cookie handle.
 * 
 * @return          The boolean value.
 */
stock bool:CookieLib_GetBool(client, Handle:cookie)
{
    // Get cookie string.
    decl String:strValue[8];
    GetClientCookie(client, cookie, strValue, sizeof(strValue));
    
    // Return the string as a boolean.
    return bool:StringToInt(strValue);
}

/**
 * Set a boolean value to a client cookie.
 * 
 * @param client    The client index.
 * @param cookie    The cookie handle.
 * @param value     The boolean value.
 */
stock CookieLib_SetBool(client, Handle:cookie, bool:value)
{
    // Convert bool to string.
    decl String:strValue[8];
    IntToString(_:value, strValue, sizeof(strValue));
    
    // Set the converted string to the cookie.
    SetClientCookie(client, cookie, strValue);
}

/**
 * Returns an integer client cookie value.
 * 
 * @param client    The client index.
 * @param cookie    The cookie handle.
 * 
 * @return          The integer value.
 */
stock CookieLib_GetInt(client, Handle:cookie)
{
    decl String:strValue[16];
    GetClientCookie(client, cookie, strValue, sizeof(strValue));
    
    // Return the string as an integer.
    return StringToInt(strValue);
}

/**
 * Set an integer value to a client cookie.
 * 
 * @param client    The client index.
 * @param cookie    The cookie handle.
 * @param value     The integer value.
 */
stock CookieLib_SetInt(client, Handle:cookie, value)
{
    // Convert value to string.
    decl String:strValue[16];
    IntToString(value, strValue, sizeof(strValue));
    
    // Set string value.
    SetClientCookie(client, cookie, strValue);
}

/**
 * Returns a float client cookie value.
 * 
 * @param client    The client index.
 * @param cookie    The cookie handle.
 * 
 * @return          The float value.
 */
stock Float:CookieLib_GetFloat(client, Handle:cookie)
{
    decl String:strValue[16];
    GetClientCookie(client, cookie, strValue, sizeof(strValue));
    
    // Return the string as an integer.
    return StringToFloat(strValue);
}

/**
 * Set a float value to a client cookie.
 * 
 * @param client    The client index.
 * @param cookie    The cookie handle.
 * @param value     The float value.
 */
stock CookieLib_SetFloat(client, Handle:cookie, Float:value)
{
    // Convert value to string.
    decl String:strValue[16];
    FloatToString(value, strValue, sizeof(strValue));
    
    // Set string value.
    SetClientCookie(client, cookie, strValue);
}

/**
 * Set an array of string values to an array of client cookies.
 * 
 * @param client        The client index.
 * @param cookies       The array of cookie handles to store each value.
 * @param numCookies    The number of cookies to get the values of.
 * @param values        The array of string values.
 */
stock CookieLib_SetArray(client, Handle:cookies[], String:values[][], numCookies)
{
    for (new cindex = 0; cindex < numCookies; cindex++)
    {
        // Set string value.
        SetClientCookie(client, cookies[cindex], values[cindex]);
    }
}

/**
 * Get an array of string values from an array of client cookies.
 * 
 * @param client        The client index.
 * @param cookies       The array of cookie handles to get each value from.
 * @param values        The array of returned string values.
 * @param numCookies    The number of cookies to get the values of.
 * @param maxlen        The max length of each value.
 */
stock CookieLib_GetArray(client, Handle:cookies[], String:values[][], numCookies, maxlen)
{
    for (new cindex = 0; cindex < numCookies; cindex++)
    {
        // Set string value.
        GetClientCookie(client, cookies[cindex], values[cindex], maxlen);
    }
}
