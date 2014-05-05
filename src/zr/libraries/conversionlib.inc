/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          conversionlib.inc
 *  Type:          Library
 *  Description:   A library for converting units into other units.
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

#if defined _conversionlib_included
 #endinput
#endif
#define _conversionlib_included

/**
 * Different types of measurement.
 * I don't add weight because in the source engine everything is a constant density.
 */
enum ConvLibMeasurements
{
    ConvLibMeasurement_Distance,
    ConvLibMeasurement_Time
}

/**
 * Units of distance.
 */
enum ConvLibDistance
{
    ConvLibDistance_GameUnit,
    ConvLibDistance_Inch,
    ConvLibDistance_Foot,
    ConvLibDistance_Yard,
    ConvLibDistance_Mile,
    ConvLibDistance_Milimeter,
    ConvLibDistance_Centimeter,
    ConvLibDistance_Meter,
    ConvLibDistance_Kilometer
}

/**
 * Units of time.
 */
enum ConvLibTime
{
    ConvLibTime_Millisecond,
    ConvLibTime_Second,
    ConvLibTime_Minute,
    ConvLibTime_Hour,
    ConvLibTime_Day,
    ConvLibTime_Week,
    ConvLibTime_Month
}

/**
 * Conversion table multiplers between all units.
 * The compiler doesn't like when I index the 2nd dimension with ConvertLibUnits.
 * I bypass this by indexing it as normal integer elements and then reading ConvertLibUnits as an int for the 2nd dimension.
 */
new const Float:g_flConversionFactors[ConvLibMeasurements][][] =
{
    // Distance conversion factors.
    {
        {   1.0, 0.75, 9.0, 27.0, 47520.0, 19.05, 1.905, 0.01905, 0.00001905                }, // Game units to x
        {   1.3333, 1.0, 0.08333, 0.027778, 0.0000157, 25.4, 2.54, 0.0254, 0.0000254        }, // Inches to x
        {   0.1111, 12.0, 1.0, 0.33333, 0.0001893, 304.8, 30.48, 0.3048, 0.0003048          }, // Feet to x
        {   0.037037, 36.0, 3.0, 1.0, 0.0005681, 914.4, 91.44, 0.9144, 0.0009144            }, // Yards to x
        {   0.000021, 93360.0, 5280.0, 1760.0, 1.0, 1609344.0, 160934.4, 1609.344, 1.609344 }, // Miles to x
        {   0.052493, 0.03937, 0.003281, 0.001094, 0.000000621, 1.0, 0.1, 0.001, 0.000001   }, // Milimeters to x
        {   0.52493, 0.3937, 0.03281, 0.01094, 0.00000621, 10.0, 1.0, 0.01, 0.00001         }, // Centimeters to x
        {   52.493, 39.37, 3.281, 1.094, 0.000621, 1000.0, 100.0, 1.0, 0.001                }, // Meters to x
        {   52493.0, 39370.0, 3281.0, 1094.0, 0.621, 1000000.0, 100000.0, 1000.0, 1.0       }  // Kilometers to x
    },
    // Time conversion factors.
    {
        {   1.0, 0.001, 0.000016666, 0.0000002777, 0.000000011574, 0.000000001653, 0.00000000038027, 0.0, 0.0 }, // Milliseconds to x
        {   1000.0, 1.0, 0.0166666, 0.00027777, 0.000011574, 0.000001653, 0.00000038027, 0.0, 0.0             }, // Seconds to x
        {   60000.0, 60.0, 1.0, 0.0166666, 0.00069444, 0.000099206, 0.00002281, 0.0, 0.0                      }, // Minutes to x
        {   3600000.0, 3600.0, 60.0, 1.0, 0.041666, 0.005952, 0.001368, 0.0, 0.0                              }, // Hours to x
        {   86400000.0, 86400.0, 1440.0, 24.0, 1.0, 0.142857, 0.03285, 0.0, 0.0                               }, // Days to x
        {   604800000.0, 604800.0, 10080.0, 168.0, 7.0, 1.0, 0.22998, 0.0, 0.0                                }, // Weeks to x
        {   2629743800.0, 2629743.8, 43829.0639, 730.484398, 30.43684, 4.34812141, 1.0, 0.0, 0.0              }, // Months to x
        { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
    }
};

/**
 * The abbreviation for each of the supported units.
 */
new const String:g_strUnits[ConvLibMeasurements][][] =
{
    // Distance units.
    {
        "game units",
        "in",
        "ft",
        "yd",
        "mi",
        "mm",
        "cm",
        "m",
        "km"
    },
    // Time conversion units.
    {
        "ms",
        "sec",
        "min",
        "hr",
        "days",
        "weeks",
        "months",
        "",
        ""
    }
};

/**
 * Convert a number of x units to a number of y units.
 * See enum ConvLib* at top of file.
 * 
 * @param measurement   The type of measurement to convert.
 * @param units_in      The units of the number being inputted.
 * @param units_out     The units to convert the number to.
 * @param number        The number with the units given in 'units_from'.
 * 
 * @return              Result with the units given in 'units_to'.
 */
stock Float:ConvertLib_Convert(ConvLibMeasurements:measurement, units_in, units_out, Float:number)
{
    return number * g_flConversionFactors[measurement][units_in][units_out];
}

/**
 * Looks up the abbreviation for a given symbol from ConvertLibUnits
 * 
 * @param measurement   The type of measurement to look up.
 * @param units         The symbol to look up abbreviation for.
 * @param abbrev        The output string for the abbreviation for the units.
 * @param maxlen        The max length of the abbreviation.
 */
stock ConvertLib_SymbolToAbbrev(ConvLibMeasurements:measurement, units, String:abbrev[], maxlen)
{
    strcopy(abbrev, maxlen, g_strUnits[measurement][units]);
}

/**
 * Looks up the abbreviation for certain units.
 * 
 * @param measurement   The type of measurement to find symbol for.
 * @param abbrev        The abbreviation to find symbol for.
 * 
 * @return              The symbol for the abbreviation or -1 if symbol can't be found.
 */
stock ConvertLib_AbbrevToSymbol(ConvLibMeasurements:measurement, const String:abbrev[])
{
    for (new uindex; uindex < sizeof(g_strUnits[]); uindex++)
    {
        if (StrEqual(abbrev, g_strUnits[measurement][uindex], false))
            return uindex;
    }
    
    return -1;
}

/**
 * Detects number and units in a string and outputs as any units.
 * 
 * @param measurement   The type of measurement to detect.
 * @param number        The number with units to convert.  E.g. "10cm" or "5 days"
 * @param assume        Units to assume if valid units can't be detected.
 * @param units_out     The units to convert to.
 * 
 * @return              The converted value.
 */
stock Float:ConvertLib_DetectAndConvert(ConvLibMeasurements:measurement, const String:number[], assume, units_out)
{
    new chr;
    new length = strlen(number);
    
    // Extract number.
    decl String:strNumber[12];
    new Float:flNumber;
    for (chr = 0; chr < length; chr++)
    {
        if (!IsCharNumeric(number[chr]) && number[chr] != '.')
        {
            strcopy(strNumber, chr + 1, number);
            flNumber = StringToFloat(strNumber);
            break;
        }
    }
    
    // Extract units.
    decl String:strUnits[16];
    strcopy(strUnits, sizeof(strUnits), number[chr]);
    TrimString(strUnits);
    
    // Find symbol for abbreviation, if it fails then use value in 'assume'
    new detected_units = ConvertLib_AbbrevToSymbol(measurement, strUnits);
    if (detected_units == -1)
        detected_units = assume;
    
    return ConvertLib_Convert(measurement, detected_units, units_out, flNumber);
}

stock ConvertLib_Test()
{
    LogMessage("*********TESTING CONVERSION LIBRARY CONVERSION FACTORS*********");
    new ConvLibMeasurements:measurement;
    new Float:converted;
    for (new i = 0; i < sizeof(g_flConversionFactors); i++)
    {
        measurement = ConvLibMeasurements:i;
        for (new j = 0; j < sizeof(g_flConversionFactors[]); j++)
        {
            for (new k = 0; k < sizeof(g_flConversionFactors[][]); k++)
            {
                if (g_strUnits[measurement][j][0] == 0 || g_strUnits[measurement][k][0] == 0)
                    continue;
                
                converted = ConvertLib_Convert(measurement, j, k, 1.0);
                LogMessage("1 %s converted to %s: %f", g_strUnits[measurement][j], g_strUnits[measurement][k], converted);
            }
        }
    }
}
