### CLEO? For Android?
Well, yes! This is a CLEO wrapped in an AML mod!
Original author of a CLEO on Android is Alexander Blade (http://www.dev-c.com/).

**Please keep in mind that you should not download random CLEO scripts! They may be malicious and may delete your game data!**

### Why does it exists?
This one allows mods made specially for AML to communicate with the CLEO. Also, it has NEW settings that you WILL LIKE!

### I miss the PC opcodes! Can i get them?
YES! YOU CAN! But not all of them.
They are implemented in this CLEOMod and are already working, there is no need to enable them somewhere. However, Sanny Builder 3 doesnt know these opcodes.
If you want to use them to compile your scripts, you need to manually add them to the configuration file.

Here is how to do this:
1. Enter the directory of Sanny Builder 3
2. Enter ../data/sa_mobile (or ../data/vc_mobile for GTA:VC Android)
3. Open and add these lines at the end of the file SASCM.ini (or VCSCM.ini):
```
0A8E=3,%3d% = %1d% + %2d% ; int
0A8F=3,%3d% = %1d% - %2d% ; int
0A90=3,%3d% = %1d% * %2d% ; int
0A91=3,%3d% = %1d% / %2d% ; int
0A93=0,terminate_this_custom_script
0A96=2,%2d% = actor %1d% struct
0A97=2,%2d% = car %1d% struct
0A98=2,%2d% = object %1d% struct
0A99=1,set_current_directory %1b:userdir/rootdir%
0A9A=3,%3d% = openfile %1d% mode %2d% // IF and SET
0A9B=1,closefile %1d%
0A9C=2,%2d% = file %1d% size
0A9D=3,readfile %1d% size %2d% to %3d%
0A9E=3,writefile %1d% size %2d% from %3d%
0A9F=1,%1d% = current_thread_pointer
0AA0=1,gosub_if_false %1p%
0AA1=0,return_if_false
0AA2=2,%2h% = load_library %1d% // IF and SET
0AA3=1,free_library %1h%
0AA4=3,%3d% = get_proc_address %1d% library %2d% // IF and SET
0AA9=0,is_game_version_original // always false, use 0DD6 (GET_GAME_VERSION) for Android
0AAA=2,%2d% = thread %1d% pointer // IF and SET
0AAB=1,file_exists %1d%
0AB1=-1,call_scm_func %1p%
0AB2=-1,ret
0AB3=2,var %1d% = %2d%
0AB4=2,%2d% = var %1d%
0AB7=2,get_vehicle %1d% number_of_gears_to %2d%
0AB8=2,get_vehicle %1d% current_gear_to %2d%
0ABA=1,terminate_all_custom_scripts_with_this_name %1d%
0ABD=1,vehicle %1d% siren_on
0ABE=1,vehicle %1d% engine_on
0ABF=2,set_vehicle %1d% engine_state_to %2d%
0AC6=2,get_label_pointer %1d% store_to %2d%
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
0AD2=2,%2d% = player %1d% targeted_actor // IF and SET
0AD3=-1,string %1d% format %2d%
0AD4=-1,%3d% = scan_string %1d% format %2d%  // IF and SET
0AD5=3,file %1d% seek %2d% from_origin %3d% // IF and SET
0AD6=1,end_of_file %1d% reached
0AD7=3,read_string_from_file %1d% to %2d% size %3d% // IF and SET
0AD8=2,write_string_to_file %1d% from %2d% // IF and SET
0AD9=-1,write_formated_text %2d% to_file %1d%
0ADA=-1,%3d% = scan_file %1d% format %2d% // IF and SET
0ADB=2,%2d% = car_model %1o% name
0ADC=1,test_cheat %1d%
0ADD=1,spawn_car_with_model %1o% at_player_location // IF and SET // custom if-set condition
0ADE=2,%2d% = text_by_GXT_entry %1d%
0ADF=2,add_dynamic_GXT_entry %1d% text %2d%
0AE0=1,remove_dynamic_GXT_entry %1d%
0AE4=1,directory_exist %1d%
0AE5=1,create_directory %1d% // IF and SET
0AE6=3,%2d% = find_first_file %1d% get_filename_to %3d% // IF and SET
0AE7=2,%2d% = find_next_file %1d% // IF and SET
0AE8=1,find_close %1d%
0AE9=1,pop_float store_to %1d% // returns 0, ARMv7 differs from x86
0AEA=2,%2d% = actor_struct %1d% handle
0AEB=2,%2d% = car_struct %1d% handle
0AEC=2,%2d% = object_struct %1d% handle
0AEE=3,%3d% = %1d% exp %2d% // all floats
0AEF=3,%3d% = log %1d% base %2d% // all floats
0AF6=-1,ret_if_false // custom 0AB2
0AF7=-1,ret_if_true // custom 0AB2
0AF8=1,save_local_vars_named %1d% // IF and SET
0AF9=1,load_local_vars_named %1d% // IF and SET
0AFA=1,delete_local_vars_save %1d% // IF and SET
0AFB=-1,save_script_vars_named %1d% // IF and SET
0AFC=-1,load_script_vars_named %1d% // IF and SET
0AFD=1,delete_script_vars_save %1d% // IF and SET
0AFE=4,%1d% = find_custom_script_named %2d% case %3d% partial %4d% check_filename %5d% // IF and SET
0AFF=1,set_compare_flag %1d%
0CB0=1,%1d% = get_language_code
0CB1=1,%1d% = get_country_code
0CB2=2,%2d% = atof %1d%
0CB3=2,%2d% = atoi %1d%
0CB4=2,get_screen_height x %1d% y %2d%
0CB5=0,is_any_finger_onscreen // IF and SET
0CB6=3,is_finger_in_area %1d% %2d% radius %3d% // IF and SET
3A00=2,%2d% = aml_has_mod_loaded %1s% // IF and SET
3A01=3,%3d% = aml_has_mod_loaded %1s% version %2s% // IF and SET
3A02=4,aml_redirect_code %1d% add_ib %2d% to %3d% add_ib %4d%
3A03=4,aml_jump_code %1d% add_ib %2d% to %3d% add_ib %4d%
3A04=3,%3d% = aml_get_branch_dest %1d% add_ib %2d%
3A05=0,aml_mls_save
3A06=1,aml_mls_has_value %1s% // IF and SET
3A07=1,aml_mls_delete_value %1s%
3A08=2,aml_mls_set_int %1s% to %2d%
3A09=2,aml_mls_set_float %1s% to %2d%
3A0A=2,aml_mls_set_string %1s% to %2s%
3A0B=3,%3d% = aml_mls_get_int %1s% default %2d%
3A0C=3,%3d% = aml_mls_get_float %1s% default %2d%
3A0D=3,%3s% = aml_mls_get_string %1s% default %2s%
3A0E=1,do_opcode_exist %1d% // IF and SET
3A0F=2,push_string %1d% to_var %2d%
3A10=3,write_float %1d% to %2d% add_ib %3d%
3A11=1,aml_vibrate %1d% ms
3A12=0,aml_stop_vibro
3A13=2,aml_show_toast %2s% longer %1d%
3A14=1,%1d% = aml_get_battery_percentage // float
3A15=1,%1d% = aml_get_android_ver
3A16=4,aml_write_hex_at %1d% add_ib %2d% from_label %3d% size %4d%
3A17=4,aml_read_hex_at %1d% add_ib %2d% to_label %3d% size %4d%
```

There is an additional opcodes for GTA:SA Android:
```
0AB5=3,store_actor %1d% closest_vehicle_to %2d% closest_ped_to %3d%
0AB6=3,store_target_marker_coords_to %1d% %2d% %3d% // IF and SET
0AE1=7,%7d% = find_actor_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_deads %6h% // IF and SET
0AE2=7,%7d% = find_vehicle_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% pass_wrecked %6h% // IF and SET
0AE3=6,%6d% = find_object_near_point %1d% %2d% %3d% in_radius %4d% find_next %5h% // IF and SET
```

If you need extensions such as IniFiles or IntOperations, they are already available! You can find them in our project's Discord (https://discord.gg/2MY7W39kBg) or get them here:

https://github.com/AndroidModLoader/GTA_CLEO_IniFiles 
https://github.com/AndroidModLoader/GTA_CLEO_IntOperations

### About CLEO5
This version of a mod contains a few CLEO5 opcodes. I dont wanna continue working on it because it's a broken ass that is not completed yet.

Opcodes from CLEO5 in this mod (except debugging ones!!!):
```
2400=3, copy_memory %1d% to %2d% size %3d%
2401=4, read_memory_with_offset %1d% offset %2d% size %3d% store_to %4d%
2402=4, write_memory_with_offset %1d% offset %2d% size %3d% value %4d%
2403=1, forget_memory %1d%
2404=1, get_script_struct_just_created %1d%
2405=1, is_script_running %1d%
2406=1, get_script_struct_from_filename %1s%
2407=3, is_memory_equal address_a %1d% address_b %2d% size %d3%
2408=1,terminate_script %1d%
2600=1, is_text_empty %1s%
2601=3, is_text_equal %1s% another %2s% ignore_case %3d%
2602=3, is_text_in_text %1s% sub_text %2s% ignore_case %3d%
2603=3, is_text_prefix %1s% prefix %2s% ignore_case %3d%
2604=3, is_text_suffix %1s% suffix %2s% ignore_case %3d%
2605=-1, display_text_formatted offset_left %1d% offset_top %2d% format %3d% args
2608=3, get_text_length %1d% store_to %2d%
2609=-1,add_text_label_formatted %1d% args %2d%
2300=2, get_file_position %1d% store_to %2d%
2301=3, read_block_from_file %1d% size %2d% buffer %3d% // IF and SET
2302=3, write_block_to_file %1d% size %2d% address %3d% // IF and SET
2303=2, %2s% = resolve_filepath %1s%
2304=3, %3s% = get_script_filename %1d% full_path %2d% // IF and SET
2305=8, get_file_write_time %1s% year %2d% month %3d% day %3d% hour %4d% minute %5d% second %6d% milisecond %7d% // IF and SET
0B00=1, delete_file %1s% // IF and SET
0B01=1, delete_directory %1s% with_all_files_and_subdirectories %2d% // IF and SET
0B02=2, move_file %1s% to %2s% // IF and SET
0B03=2, move_directory %1s% to %2s% // IF and SET
0B04=2, copy_file %1s% to %2s% // IF and SET
0B05=2, copy_directory %1d% to %2d% // IF and SET
2000=1, %1d% = get_cleo_arg_count
2002=-1, cleo_return_with ...
2003=-1, cleo_return_fail
2700=2, is_bit_set value %1d% bit_index %2d%
2701=2, set_bit value %1d% bit_index %2d%
2702=2, clear_bit value %1d% bit_index %2d%
2703=3, toggle_bit value %1d% bit_index %2d% state %3d%
2704=1, is_truthy value %1d%
2705=-1, pick_random_int values %d% store_to %d%
2706=-1, pick_random_float values %d% store_to %d%
2707=-1, pick_random_text values %d% store_to %d%
2708=1, random_chance %1d%
```

### Another opcodes
I also moved MathOperations into the CLEOMod itself! Starting with 2.0.1.7, there is new math opcodes:
```
1C00=2,%2d% = to_radian %1d%
1C01=2,%2d% = to_degree %1d%
1C02=3,%3d% = modulo_int %1d% %2d%
1C03=3,%3d% = modulo_float %1d% %2d%
1C04=2,%2d% = acos %1d%
1C05=2,%2d% = asin %1d%
1C06=2,%2d% = atan %1d%
1C07=2,%2d% = cbrt %1d%
1C08=2,%2d% = ceil %1d%
1C09=2,%2d% = cos %1d%
1C10=2,%2d% = cosh %1d%
1C11=2,%2d% = expm1 %1d%
1C12=3,%3d% = fdim %1d% %2d%
1C13=2,%2d% = floor %1d%
1C14=3,%3d% = hypot %1d% %2d%
1C15=4,%4d% = fma %1d% %2d% %3d%
1C16=3,%3d% = fmax %1d% %2d%
1C17=3,%3d% = fmin %1d% %2d%
1C18=2,%2d% = sin %1d%
1C19=2,%2d% = sinh %1d%
1C20=2,%2d% = tan %1d%
1C21=2,%2d% = tanh %1d%
1C22=3,%3d% = atan2 %1d% %2d%
1C23=3,%2d% exp %3d% = frexp %1d%
1C24=3,%3d% = ldexp %1d% exp %2d%
1C25=3,%2d% intpart %3d% = modf %1d%
1C26=3,%3d% = scalbn %1d% int_n %2d%
1C27=2,%2d% = trunc %1d%
1C28=3,%3d% = remainder %1d% %2d%
1C29=2,%2d% = fpclassify %1d%
1C30=4,%4d% = clamp_float %1d% limit %2d% %3d%
1C31=4,%4d% = clamp_int %1d% limit %2d% %3d%
1C32=7,%7d% = distance_from %1d% %2d% %3d% to %4d% %5d% %6d%
1C33=3,%3d% = distance_from %1d% to_vec %2d%
1C34=5,%5d% = distance2d_from %1d% %2d% to %3d% %4d%
1C35=3,%3d% = distance2d_from %1d% to_vec %2d%
1C36=2,%2d% = invsqrt %1d%
1C37=2,%2d% = tgamma %1d%
1C38=2,%2d% = lgamma %1d%
1C39=3,%3d% = remquo %1d% %2d%
1C40=2,%2d% = exp %1d%
1C41=2,%2d% = exp2 %1d%
1C42=2,%2d% = erf %1d%
1C43=2,%2d% = erfc %1d%
1C44=3,%3d% = nextafter_from %1d% to %2d%
1C45=3,%3d% = nexttoward_from %1d% to %2d%
1C46=3,%3d% = copysign %1d% %2d%
1C47=1,toggle_bool %1d%
1C48=2,minmax_shuffle %1d% %2d% // IF and SET
1C49=3,minmax_shuffle3 %1d% %2d% %3d% // IF and SET
1C50=2,%2d% = logb %1d%
1C51=2,%2d% = ilogb %1d%
1C52=2,%2d% = signbit %1d%
1C53=4,%4d% = lerp %1d% %2d% t %3d%
1C54=3,%3d% = pytha %1d% %2d%
1C55=4,%4d% = r_of_t %1d% %2d% %3d%
1C56=4,%4d% = unlerp %1d% %2d% v %3d%
1C57=4,%4d% = smoothstep %1d% %2d% x %3d%
1C58=4,%4d% = smootherstep %1d% %2d% x %3d%
1C59=2,%2d% = inv_smoothstep %1d%
1C60=2,%2d% = normalize_angle %1d%
1C61=2,%2d% = normalize_radians %1d%
```