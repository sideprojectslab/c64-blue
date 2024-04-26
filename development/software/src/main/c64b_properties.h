//----------------------------------------------------------------------------//
//         .XXXXXXXXXXXXXXXX.  .XXXXXXXXXXXXXXXX.  .XX.                       //
//         XXXXXXXXXXXXXXXXX'  XXXXXXXXXXXXXXXXXX  XXXX                       //
//         XXXX                XXXX          XXXX  XXXX                       //
//         XXXXXXXXXXXXXXXXX.  XXXXXXXXXXXXXXXXXX  XXXX                       //
//         'XXXXXXXXXXXXXXXXX  XXXXXXXXXXXXXXXXX'  XXXX                       //
//                       XXXX  XXXX                XXXX                       //
//         .XXXXXXXXXXXXXXXXX  XXXX                XXXXXXXXXXXXXXXXX.         //
//         'XXXXXXXXXXXXXXXX'  'XX'                'XXXXXXXXXXXXXXXX'         //
//----------------------------------------------------------------------------//
//             Copyright 2023 Vittorio Pascucci (SideProjectsLab)             //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// You may obtain a copy of the License at                                    //
//                                                                            //
//     http://www.apache.org/licenses/LICENSE-2.0                             //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS,          //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//----------------------------------------------------------------------------//

#ifndef C64B_PROPERTIES_H
#define C64B_PROPERTIES_H

#include "stdint.h"
#include "uni_property.h"
#include "c64b_keyboard.h"

void     c64b_property_reset(void);
void     c64b_property_init(void);
void     c64b_property_set_u8(const char* key, uint8_t value);
uint8_t  c64b_property_get_u8(const char* key, uint8_t def);

#define KB_MAP_SYMBOLIC   0
#define KB_MAP_POSITIONAL 1

#define C64B_PROPERTY_KEY_KB_MAP "c64b.kb_map"

typedef enum
{
	CT_MAP_IDX_BY = 0,
	CT_MAP_IDX_BH,
	CT_MAP_IDX_BM,
	CT_MAP_IDX_LT,
	CT_MAP_IDX_RT,
	CT_MAP_IDX_LS,
	CT_MAP_IDX_RS,
	CT_MAP_IDX_NUM
} t_c64b_ct_map_idx;

#define C64B_PROPERTY_KEY_CT_BY  "c64b.ct_by"
#define C64B_PROPERTY_KEY_CT_BH  "c64b.ct_bh"
#define C64B_PROPERTY_KEY_CT_BM  "c64b.ct_bm"
#define C64B_PROPERTY_KEY_CT_LT  "c64b.ct_lt"
#define C64B_PROPERTY_KEY_CT_RT  "c64b.ct_rt"
#define C64B_PROPERTY_KEY_CT_LS  "c64b.ct_ls"
#define C64B_PROPERTY_KEY_CT_RS  "c64b.ct_rs"

extern unsigned int kb_map;
extern unsigned int ct_map[CT_MAP_IDX_NUM];

#endif
