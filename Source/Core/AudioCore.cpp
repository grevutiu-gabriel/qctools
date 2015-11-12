/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <Core/AudioCore.h>
#include <cfloat>

const struct per_group AudioPerGroup [Group_AudioMax]=
{
    //R128
    {
        Item_R128M,             1,  -70,    0,  3,  "R.128",  true,
        "R 128 refers to a European Broadcasting Union (EBU) specification\n"
        "document governing several loudness parameters, including momentary,\n"
        "integrated, and short-term loudness. QCTools specifically examines momentary\n"
        "loudness, or sudden changes in volume over brief intervals of time (up to 400ms).\n"
        "This can be helpful in identifying areas where volume may exceed upper loudness\n"
        "tolerance levels as perceived by an audience.\n",
    },
    //astats levels
    {
        Item_DC_offset,             3,   -1,    1,  3,  "Levels",  true,
        "(TODO)\n",
    },
    //astats diff
    {
        Item_Min_difference,        3,    0,    1,  3,  "Aud Diffs", true,
        "(TODO)\n",
    },
    //astats rms
    {
        Item_Peak_level,            3,  -70,    0,  3,  "RMS",      true,
        "(TODO)\n",
    },
    //LRA
    //{
    //    Item_LRAL,       3,    0,    0,  3,  "LRA",  true,
    //    "(TODO)\n",
    //},
};

const struct per_item AudioPerItem [Item_AudioMax]=
{
    //Y
    { Group_R128,         Group_AudioMax,   "R128.M",     "lavfi.r128.M",                         3,   false,  DBL_MAX, DBL_MAX },
    { Group_astats_levels,Group_AudioMax,   "DC_offset",  "lavfi.astats.Overall.DC_offset",       3,   false,  DBL_MAX, DBL_MAX },
    { Group_astats_levels,Group_AudioMax,   "Min_level",  "lavfi.astats.Overall.Min_level",       3,   false,  DBL_MAX, DBL_MAX },
    { Group_astats_levels,Group_AudioMax,   "Max_level",  "lavfi.astats.Overall.Max_level",       3,   false,  DBL_MAX, DBL_MAX },
    { Group_adif,         Group_AudioMax,   "ADIFMin",    "lavfi.astats.Overall.Min_difference",  3,   false,  DBL_MAX, DBL_MAX },
    { Group_adif,         Group_AudioMax,   "ADIFMax",    "lavfi.astats.Overall.Max_difference",  3,   false,  DBL_MAX, DBL_MAX },
    { Group_adif,         Group_AudioMax,   "ADIFMean",   "lavfi.astats.Overall.Mean_difference", 3,   false,  DBL_MAX, DBL_MAX },
    { Group_astats_RMS,   Group_AudioMax,   "Peak Level", "lavfi.astats.Overall.Peak_level",      3,   false,  DBL_MAX, DBL_MAX },
    { Group_astats_RMS,   Group_AudioMax,   "RMS_peak",   "lavfi.astats.Overall.RMS_peak",        3,   false,  DBL_MAX, DBL_MAX },
    { Group_astats_RMS,   Group_AudioMax,   "RMS_trough", "lavfi.astats.Overall.RMS_trough",      3,   false,  DBL_MAX, DBL_MAX },
    //{ Group_R128,   Group_AudioMax,       "R128.S",         "lavfi.r128.S",             0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_R128,   Group_AudioMax,       "R128.I",         "lavfi.r128.I",             0,   true,   DBL_MAX, DBL_MAX },
    //U
    //{ Group_LRA,    Group_AudioMax,       "LRAL",           "lavfi.r128.LRA.low",       0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_LRA,    Group_AudioMax,       "LRA",            "lavfi.r128.LRA",           0,   false,  DBL_MAX, DBL_MAX },
    //{ Group_LRA,    Group_AudioMax,       "LRAH",           "lavfi.r128.LRA.high",      0,   true,   DBL_MAX, DBL_MAX },
};
