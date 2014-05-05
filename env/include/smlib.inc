#if defined _smlib_included
	#endinput
#endif
#define _smlib_included

// Changes by Richard:
// * Added "smlib_" prefix on included files to avoid file name conflicts.
// * Line endings converted to LF.
//
// Note: Compiler hangs when the entire smlib is included. Including individual
//       files where needed works fine.
//
// Actually it's 0.10.2. Seems like the developer forgot to update this one.
#define SMLIB_VERSION "0.9.7"

#include <smlib/smlib_general>

#include <smlib/smlib_arrays>
#include <smlib/smlib_clients>
#include <smlib/smlib_colors>
#include <smlib/smlib_concommands>
#include <smlib/smlib_convars>
#include <smlib/smlib_crypt>
#include <smlib/smlib_debug>
#include <smlib/smlib_dynarrays>
#include <smlib/smlib_edicts>
#include <smlib/smlib_effects>
#include <smlib/smlib_entities>
#include <smlib/smlib_files>
#include <smlib/smlib_game>
#include <smlib/smlib_math>
#include <smlib/smlib_server>
#include <smlib/smlib_strings>
#include <smlib/smlib_sql>
#include <smlib/smlib_teams>
#include <smlib/smlib_vehicles>
#include <smlib/smlib_weapons>
#include <smlib/smlib_world>
