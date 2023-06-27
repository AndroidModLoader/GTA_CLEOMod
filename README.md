### CLEO? For Android?
Well, yes! This is a CLEO wrapped in an AML mod!
Original author of a CLEO on Android is Alexander Blade (http://www.dev-c.com/).

### Why does it exists?
This one allows mods made specially for AML to communicate with the CLEO. Also, it has NEW settings that you WILL LIKE!

### I miss the PC opcodes! Can i get them?
YES! YOU CAN! But not all of them!
They are implemented in this CLEOMod and are already working, there is no need to enable them somewhere. However, Sanny Builder 3 doesnt know these opcodes, if you want to use them with it, you need to manually add them to the configuration file.

Here is how to do this:
1. Enter the directory of Sanny Builder 3
2. Enter ../data/sa_mobile (or ../data/vc_mobile for GTA:VC Android)
3. Open and add these lines at the end of the file SASCM.ini (or VCSCM.ini):
```
0A96=2,%2d% = actor %1d% struct
0A97=2,%2d% = car %1d% struct
0A98=2,%2d% = object %1d% struct
0A9F=1,%1d% = current_thread_pointer
0AAA=2,%2d% = thread %1d% pointer // IF and SET
0AC7=2,%2d% = var %1d% offset
0AC8=2,%2d% = allocate_memory_size %1d%
0AC9=1,free_allocated_memory %1d%
0AEA=2,%2d% = actor_struct %1d% handle
0AEB=2,%2d% = car_struct %1d% handle
0AEC=2,%2d% = object_struct %1d% handle
0AEE=3,%3d% = %1d% exp %2d% // all floats
0AEF=3,%3d% = log %1d% base %2d% // all floats
```

There is additional opcodes for GTA:SA Android:
```
0AB6=3,store_target_marker_coords_to %1d% %2d% %3d% // IF and SET
0AB7=2,get_vehicle %1d% number_of_gears_to %2d%
0AB8=2,get_vehicle %1d% current_gear_to %2d%
0ABD=1,vehicle %1d% siren_on
0ABE=1,vehicle %1d% engine_on
0ADB=2,%2d% = car_model %1o% name
0AE1=7,%7d% = find_actor_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_deads %6h% //IF and SET
0AE2=7,%7d% = find_vehicle_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_wrecked %6h% //IF and SET
0AE3=6,%6d% = find_object_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% //IF and SET

```

If you need extensions such as IniFiles or IntOperations, they are already available! You can find them in our project's Discord (https://discord.gg/2MY7W39kBg) or get them here:

https://github.com/AndroidModLoader/GTA_CLEO_IniFiles 
https://github.com/AndroidModLoader/GTA_CLEO_IntOperations