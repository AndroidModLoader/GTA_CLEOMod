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
0A8E=3,%3d% = %1d% + %2d% ; int
0A8F=3,%3d% = %1d% - %2d% ; int
0A90=3,%3d% = %1d% * %2d% ; int
0A91=3,%3d% = %1d% / %2d% ; int
0A96=2,%2d% = actor %1d% struct
0A97=2,%2d% = car %1d% struct
0A98=2,%2d% = object %1d% struct
0A9F=1,%1d% = current_thread_pointer
0AA0=1,gosub_if_false %1p%
0AA1=0,return_if_false
0AA2=2,%2h% = load_library %1d% // IF and SET
0AA4=3,%3d% = get_proc_address %1d% library %2d% // IF and SET
0AAA=2,%2d% = thread %1d% pointer // IF and SET
0AAB=1,file_exists %1d%
0AB1=-1,call_scm_func %1p%
0AB2=-1,ret
0AC6=2,push_string %1d% var %2d% // This one is custom to be used with 0AC8
0AC7=2,%2d% = var %1d% offset
0AC8=2,%2d% = allocate_memory_size %1d%
0AC9=1,free_allocated_memory %1d%
0ACA=1,show_text_box %1d%
0ACB=3,show_styled_text %1d% time %2d% style %3d%
0ACC=2,show_text_lowpriority %1d% time %2d%
0ACD=2,show_text_highpriority %1d% time %2d%
0ACE=-1,show_formatted_text_box %1d%
0ACF=-1,show_formatted_styled_text %1d% time %2d% style %3d%
0AD0=-1,show_formatted_text_lowpriority %1d% time %2d%
0AD1=-1,show_formatted_text_highpriority %1d% time %2d%
0AD2=2,%2d% = player %1d% targeted_actor //IF and SET
0AD3=-1,string %1d% format %2d%
0AD4=-1,%3d% = scan_string %1d% format %2d%  //IF and SET
0ADB=2,%2d% = car_model %1o% name
0ADD=1,spawn_car_with_model %1o% at_player_location //IF and SET // custom if-set condition
0ADE=2,%2d% = text_by_GXT_entry %1d%
0ADF=2,add_dynamic_GXT_entry %1d% text %2d%
0AE0=1,remove_dynamic_GXT_entry %1d%
0AE4=1,directory_exist %1d%
0AEA=2,%2d% = actor_struct %1d% handle
0AEB=2,%2d% = car_struct %1d% handle
0AEC=2,%2d% = object_struct %1d% handle
0AEE=3,%3d% = %1d% exp %2d% // all floats
0AEF=3,%3d% = log %1d% base %2d% // all floats
0AF6=-1,ret_if_false // custom 0AB2
0AF7=-1,ret_if_true // custom 0AB2
0AF8=1,save_local_vars_named %1d% //IF and SET
0AF9=1,load_local_vars_named %1d% //IF and SET
0AFA=1,delete_local_vars_save %1d% //IF and SET
0AFB=-1,save_script_vars_named %1d% //IF and SET
0AFC=-1,load_script_vars_named %1d% //IF and SET
0AFD=1,delete_script_vars_save %1d% //IF and SET
BA00=2,%2d% = aml_has_mod_loaded %1s% // IF and SET
BA01=3,%3d% = aml_has_mod_loaded %1s% version %2s% // IF and SET
BA02=4,aml_redirect_code %1d% add_ib %2d% to %3d% add_ib %4d%
```

There is an additional opcodes for GTA:SA Android:
```
0AB5=3,store_actor %1d% closest_vehicle_to %2d% closest_ped_to %3d%
0AB6=3,store_target_marker_coords_to %1d% %2d% %3d% // IF and SET
0AB7=2,get_vehicle %1d% number_of_gears_to %2d%
0AB8=2,get_vehicle %1d% current_gear_to %2d%
0ABD=1,vehicle %1d% siren_on
0ABE=1,vehicle %1d% engine_on
0ABF=2,set_vehicle %1d% engine_state_to %2d%
0AE1=7,%7d% = find_actor_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_deads %6h% //IF and SET
0AE2=7,%7d% = find_vehicle_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_wrecked %6h% //IF and SET
0AE3=6,%6d% = find_object_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% //IF and SET

```

If you need extensions such as IniFiles or IntOperations, they are already available! You can find them in our project's Discord (https://discord.gg/2MY7W39kBg) or get them here:

https://github.com/AndroidModLoader/GTA_CLEO_IniFiles 
https://github.com/AndroidModLoader/GTA_CLEO_IntOperations