/****************************************************************************
 *    Copyright 2010 Andy Kirkham, Stellar Technologies Ltd
 *    
 *    This file is part of the Satellite Observers Workbench (SOWB).
 *
 *    SOWB is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    SOWB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SOWB.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    $Id: main.cpp 5 2010-07-12 20:51:11Z ajk $
 *    
 ***************************************************************************/
 
#include "stations.h"

/** COSPAR observing stations around the World. 
 */
const STATION_t stations[] = {
    
    { 2010, "MM",   30.3340,     -97.7610,   160.,  "5105 Crestway Dr." },
    { 2011, "MM",   30.3150,     -97.8661,   300.,  "Bee Caves Rsrch Ctr" },
    { 2420, "RE",   55.9486,      -3.1386,    40.,  "Russell Eberst" },
    { 2675, "DB",   52.1358,      -2.3264,    70.,  "David Brierley" },
    { 2676, "DB",   52.1273,      -2.3365,   107.,  "David Brierley" },
    { 2018, "PW",   51.0945,      -1.1188,   150.,  "Peter Wakelin" },
    { 2701, "TM",   43.6876,     -79.3924,   230.,  "Ted Molczan" },
    { 3022, "JC",   48.7389,       3.4589,    52.,  "Jean-Paul Cornec" },
    { 5917, "BG",   59.3418,      18.0545,    33.,  "Bjorn Gimle" },
    { 5918, "BG",   59.2985,      18.1045,    44.,  "Bjorn Gimle" },
    { 5919, "BG",   59.2615,      18.6206,    33.,  "Bjorn Gimle" },
    { 9987, "BF",   59.3418,      18.0545,    30.,  "Bjorn Gimle" },
    {  100, "SG",   59.4628,      17.9137,    30.,  "Sven Grahn" },
    { 8305, "PG",   26.2431,     -98.2163,    30.,  "Paul Gabriel" },
    { 2563, "PN",   51.0524,       2.4043,    10.,  "Pierre Nierinck" },
    { 6226, "SC",   28.4861,     -97.8194,   110.,  "Scott Campbell" },
    { 8539, "SN",   39.4707,     -79.3388,   839.,  "Steve Newcomb" },
    { 2751, "BM",   51.3440,      -1.9849,   125.,  "Bruce MacDonald" },
    { 2756, "AK",   56.0907,      -3.1623,    25.,  "Andy Kirkham" }, /* wow, that's me! Shame I no longer live there. */
    {  433, "GR",  -33.9406,      18.5129,    10.,  "Greg Roberts" },
    { 4541, "AR",   41.9639,      12.4531,    80.,  "Alberto Rango" },
    { 4542, "AR",   41.9683,      12.4545,    80.,  "Alberto Rango" },
    { 4641, "AR",   41.1060,      16.9010,    70.,  "Alberto Rango" },
    { 2115, "MW",   51.3286,       0.7950,    75.,  "Mike Waterman" },
    { 1775, "KF",   44.6062,     -75.6910,   200.,  "Kevin Fettner" },
    { 1747, "DD",   45.7275,     -72.3526,   191.,  "Daniel Deak" },
    { 8597, "TB",  -34.9638,     138.6333,   100.,  "Tony Beresford" }, 
    { 8730, "EC",   30.3086,     -97.7279,   150.,  "Ed Cannon" },
    { 9730, "MM",   30.3150,     -97.8660,   280.,  "BCRC (0002)" },
    { 4353, "ML",   52.1541,       4.4908,     0.,  "Marco Langbroek" },
    { 4354, "ML",   52.1168,       4.5602,    -2.,  "Marco Langbroek" },
    {  710, "LS",   52.3261,      10.6756,    85.,  "Lutz Schindler" },
    { 1056, "MK",   57.0122,      23.9833,     4.,  "Martins Keruss" },
    {  110, "LK",   32.5408,     -96.8906,   200.,  "Lyn Kennedy" },
    {   11, "VA",   44.7269,      34.0167,   580.,  "Crimea Astrophysical Observ." },
    {   70, "BC",   53.2233,      -0.6003,    30.,  "Bob Christy" },
    { 8335, "BY",   35.8311,     -96.1471,   335.,  "Brad Young" },
    { 8336, "BY",   36.1397,     -95.9838,   205.,  "Brad Young" },
    { 8337, "BY",   36.9557,     -96.5518,   395.,  "Brad Young" },
    { 4160, "BD",   51.2793,       5.4768,    35.,  "Bram Dorreman" },
    { 9011, "RM",   50.9310,       2.4053,    72.,  "Richard Miles" },
    {   20, "PM",   50.7453,       2.1107,    70.,  "Paul Marsh" },
    {   40, "IR",   50.7453,       2.1107,    70.,  "Ian Roberts" },
    {   90, "RF",   50.7453,       2.1107,    70.,  "Richard Flagg" },
    {    1, "AK",   56.1923,      -3.0340,    53.,  "SOWB Test Station" },       /* Used for testing the SOWB. */
    {    0, "uk",    0.0000,       0.0000,     0.,  "Unknown station location" } /* Always the last entry. */
};

/** cospar_station_at
 *
 * Given a latitude and longitude is the point within a +/-0.0005 degree
 * box of a given station? If so, return a handle (array index) for that
 * station.
 *
 * @param double latitude
 * @param double longitude
 * @return int index
 */
int cospar_station_at(double latitude, double longitude) {
    int i;
    double high, low;
    
    for (i = 0; stations[i].cospar != 0; i++) {
        high = stations[i].latitude + 0.0005;
        low  = stations[i].latitude - 0.0005;
        if (latitude <= high && latitude >= low) {
            high = stations[i].longitude + 0.0005;
            low  = stations[i].longitude - 0.0005;
            if (longitude <= high && longitude >= low) {
                return i;
            }
        }
    }
    
    return i;
}

/** station
 *
 * Given an array index value, return a pointer to the station information structure.
 * The proceedure is basically a look-up opertaion.
 *
 * @param int index
 * @return STATION_t *  
 */
const STATION_t * cospar_station(int index) {
    return &stations[index];
}
