<details>
  <summary><h3>CLEO? For Android?</h3></summary>
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
0CB7=4,is_finger_in_area_timed %1d% %2d% radius %3d% time_ms %4d% // IF and SET
0CB8=4,%3d% %4d% = touchxy_to_perc %1d% %2d%
0CB9=4,%3d% %4d% = spritexy_to_perc %1d% %2d%
0CBA=1,%1d% = get_max_points_num
0CBB=3,%2d% %3d% = get_pointer_xy %1d%
0CBC=4,is_finger %1d% in_area %2d% %3d% radius %4d% // IF and SET
0CBD=5,is_finger %1d% in_area_timed %2d% %3d% radius %4d% time_ms %5d% // IF and SET
0CD0=1,has_vehicle_radio %1d% // IF and SET
0CD1=1,has_vehicle_struct_radio %1d% // IF and SET
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
</details>

# [Grimoire - Documentation](https://youtube.com/@MatiDragon)

This is a pack of opcodes to reduce lines of code that are commonly repeated in scripts. It also aims to prevent you from having to write a lot of code in your projects in order to reduce the number of variables you have to use for a simple task.

<details>
  <summary>For your Sanny Builder</summary>

SASCM.ini
```
7000=5,set_widget_transform %1d% coords %2d% %3d% scales %4d% %5d%
7001=1,get_widget_transform %1d% coords %2d% %3d% scales %4d% %5d%
7003=2,file_rename %1d% to %2d%
7004=1,create_file_or_directory %1d%

7005=3,%3d% = angle_diff %1d% %2d%
701C=2,%2d% = !! %1d%
7006=2,%2d% = ! %1d%
7002=3,%1d% = %1d% || %2d%
7007=3,%3d% = %1d% / %2d% ; float
7008=3,%3d% = %1d% * %2d% ; float
7009=3,%3d% = %1d% + %2d% ; float
700A=3,%3d% = %1d% - %2d% ; float
700B=4,%3d% %4d% = split_float_to_signed_parts %1d% decimals %2d%
700C=8,%5d% %6d% %7d% = convert_model_color %1d% inputs %2d% %3d% %4d%

700D=6,%6d% = int %1d% op %2d% int %3d% ? any_value %4d% : any_value %5d%
700E=6,%6d% = float %1d% op %2d% float %3d% ? any_value %4d% : any_value %5d%
7014=4,%4d% = is_truthy %1d% ? any_value %2d% : any_value %3d%

700F=5,%5d% = pack_set_byte %1d% byteIndex %2d% newValue %3d% isSigned %4b%
7010=4,%4d% = pack_get_byte %1d% byteIndex %2d% isSigned %3b%
7011=4,%4d% = pack_rotate %1d% direction %2b% amount %3d%
7012=4,%4d% = pack_check_truthy %1d% mask %2d% mode %3b% //IF/SET
7013=6,%6d% = pack_swap_custom %1d% i3 %2d% i2 %3d% i1 %4d% i0 %5d%
7015=6,%6d% = pack_4dec_to_int32 %1d% %2d% %3d% %4d% flags %5d%
7016=6,%3d% %4d% %5d% %6d% = unpack_int32_to_4dec %1d% flags %2d%

7017=7,%6d% %7d% = orbit_circle %1b:angle/radian% angle %2d% radius %3d% coords %4d% %5d%
7019=9,%8d% %9d% = orbit_oval %1b:angle/radian% angle %2d% radius %3d% %4d% rotation %5d% coords %6d% %7d%
701F=11,%9d% %10d% = orbit_square %1b:angle/radian% angle %2d% size %3d% %4d% smooth %5d% rotZ %6d% coords %7d% %8d%
7018=10,%8d% %9d% %10d% = orbit_sphere %1b:angle/radian% angles %2d% %3d% radius %4d% coords %5d% %6d% %7d%
701A=15,%13d% %14d% %15d% = orbit_ovoid %1b:angle/radian% angles %2d% %3d% radius %4d% %5d% %6d% rotation %7d% %8d% %9d% coords %10d% %11d% %12d%
701B=15,%13d% %14d% %15d% = orbit_cylinder %1b:angle/radian% angle %2d% level %3d% height %4d% radii %5d% %6d% rotation %7d% %8d% %9d% coords %10d% %11d% %12d%
701D=14,%12d% %13d% %14d% = orbit_polygon %1b:angle/radian% angle %2d% sides %3d% radius %4d% smooth %5d% rotation %6d% %7d% %8d% coords %9d% %10d% %11d%
701E=16,%14d% %15d% %16d% = orbit_cube %1b:angle/radian% angles %2d% %3d% size %4d% %5d% %6d% smooth %7d% rotation %8d% %9d% %10d% coords %11d% %12d% %13d%

7020=12,%9d% %10d% %11d% progress %12d% = move_lerp %1d% %2d% %3d% to %4d% %5d% %6d% deltatime %7d% speed %8d%
7021=12,%10d% %11d% %12d% progress %9d% = move_lerp_continuous %1d% %2d% %3d% to %4d% %5d% %6d% deltatime %7d% speed %8d%
7022=12,%10d% %11d% %12d% progress %9d% = move_lerp_continuous_loop %1d% %2d% %3d% to %4d% %5d% %6d% deltaTime %7d% speed %8d%
7023=6,%5d% progress %6d% = value_lerp %1d% to %2d% deltatime %3d% speed %4d%
7024=6,%5d% progress %6d% = value_lerp_continuous %1d% to %2d% deltatime %3d% speed %4d%
7025=6,%5d% progress %6d% = value_lerp_continuous_loop %1d% to %2d% deltatime %3d% speed %4d%
```

consts.txt
```
const
    // Red Green Blue
    // Hue Saturation Value
    // Hue Saturation Lightness
    COLOR_RGB_TO_HSV = 0
    COLOR_RGB_TO_HSL = 1
    COLOR_HSL_TO_HSV = 2
    COLOR_HSL_TO_RGB = 3
    COLOR_HSV_TO_HSL = 4
    COLOR_HSV_TO_RGB = 5

    OP_EQUAL = 0           // ==
    OP_UNEQUAL = 1         // !=
    OP_LESSER = 2          // <
    OP_LESSER_EQUAL = 3    // <=
    OP_GREATER = 4         // >
    OP_GREATER_EQUAL = 5   // >=

    DIRECTION_LEFT = 0
    DIRECTION_RIGTH = 1

    MODE_AND = 0
    MODE_OR = 1
end
```

keywords.txt
```ini
; Grimoire
7000=SET_WIDGET_TRANSFORM
7001=GET_WIDGET_TRANSFORM
7002=LOGICAL_OR
7003=FILE_RENAME
7004=CREATE_FILE_OR_DIRECTORY
7005=ANGLE_DIFF
7006=TOGGLE_BOOLEAN_VAR
7007=FLOAT_DIV
7008=FLOAT_MUL
7009=FLOAT_SUM
700A=FLOAT_SUB
700B=SPLIT_FLOAT_TO_SIGNED_PARTS
700C=CONVERT_MODEL_COLOR
700D=IF_TERNARY_INT
700E=IF_TERNARY_FLOAT
700F=PACK_SET_BYTE
7010=PACK_GET_BYTE
7011=PACK_ROTATE
7012=PACK_CHECK_TRUTHY
7013=PACK_SWAP_CUSTOM
7014=IF_TERNARY
7015=PACK_4DEC_TO_INT32
7016=UNPACK_INT32_TO_4DEC
7017=ORBIT_CIRCLE
7018=ORBIT_SPHERE
7019=ORBIT_OVAL
701A=ORBIT_OVOID
701B=ORBIT_CYLINDER
701C=TOGGLE_BOOLEAN_REAL
701D=ORBIT_POLYGON
701E=ORBIT_CUBE
701F=ORBIT_SQUARE
7020=MOVE_LERP
7021=MOVE_LERP_CONTINUOUS
7022=MOVE_LERP_CONTINUOUS_LOOP
7023=VALUE_LERP
7024=VALUE_LERP_CONTINUOUS
7025=VALUE_LERP_CONTINUOUS_LOOP
```

</details>

## 🌀 Orbit System

Create any orbital shape you want: circles, spheres, ovals, polygons, cubes, rectangles, and more.

This system allows you to position a point in 2D or 3D by describing orbits with different geometries.
Just specify:

* whether you use angles or radians,
* the radius or dimensions,
* the position angles,
* the optional rotations,
* and the center coordinates.

Ideal for widgets, visual effects, decorations, dynamic HUD, and complex animations.

### List 2D
```js
7017: 0@ 1@ = orbit_circle 0 angle 28.0 radius 5.0 put_at 22.454 44.312
7019: 0@ 1@ = orbit_oval 0 angle 28.0 radius 5.0 2.0 rot 0.0 put_at 22.454 44.312
701F: 0@ 1@ = orbit_square 0 angles 28.0 size 5.0 2.0 smooth 0.25 rot 0.0 put_at 22.454 44.312
```
### List 3D
```js
7018: 0@ 1@ 2@ = orbit_sphere 0 angles 28.0 90.0 radius 5.0 put_at 22.454 44.312 7.0
701A: 0@ 1@ 2@ = orbit_ovoid 0 angles 28.0 90.0 radius 5.0 2.0 10.0 rot_xyz 0.0 45.0 90.0 put_at 22.454 44.312 7.0
701B: 0@ 1@ 2@ = orbit_cylinder 0 angle 28.0 level 5.0 height 10.0 radii 2.5 10.0 rot_xyz 0.0 45.0 90.0 put_at 22.454 44.312 7.0
701D: 0@ 1@ 2@ = orbit_polygon 0 angle 28.0 sides 3 radius 2.5 smooth 0.25 rot_xyz 0.0 45.0 90.0 put_at 22.454 44.312 7.0
701E: 0@ 1@ 2@ = orbit_cube 0 angles 28.0 90.0 size 5.0 2.0 10.0 smooth 0.25 rot_xyz 0.0 45.0 90.0 put_at 22.454 44.312 7.0
```

**Input:**
* **angleMode** (0 = degrees, 1 = radians).
* **angle** / **angles** : Position angles used by the shape.
* **radius** : Main orbit radius.
* **radii** : Initial and final radius.
* **size** : Dimensions for squares, cubes, or ovoids.
* **sides** : Number of sides for the polygon.
* **smooth** : Corner smoothing (0.0 = sharp, 1.0 = rounded).
* **rot** : Rotation applied to final point.
* **put_at** : Center of the orbit.

**Output:**
* **orbit** : These are the coordinates where our point is centered.

## 📦 Bit-Packing System

Tools for **packing**, **rotating**, **extracting**, and **checking** values within an `int32`.
They allow you to save variables, optimize memory, and manipulate bytes/nibbles as if they were pieces of an elegant puzzle.

Compact 4 small values into a single 32-bit integer.

* **Unsigned** : `0 … 255`
* **Signed** : `-127 … 128` (with sign if `flags` allow)

Ideal for storing RGBA, compressed data, or any set of 4 small numbers.

```JS
7015: 0@ = pack_4dec_to_int32 255 0 0 15 flags 0b0000 // 0@ = 0xFF00000F
7015: 0@ = pack_4dec_to_int32 -127 -127 128 128 flags 0b1001 // 0@ = 0xC87F8080
7016: 1@ 2@ 3@ 4@ = unpack_int32_to_4dec 0@ flags 0b0000 // 1@ 2@ 3@ 4@ = 255 0 0 15

700F: 0@ = pack_set_byte 0@ byteIndex 1 newValue 2 isSigned false // 0@ = 0xFF02000F
7010: 6@ = pack_get_byte 0@ byteIndex 2 isSigned false // 6@ = 0

DIRECTION_LEFT = 0
DIRECTION_RIGTH = 1
7011: 0@ = pack_rotate 0@ direction DIRECTION_LEFT amount 1 // 0@ = 0x02000FFF
7011: 0@ = pack_rotate 0@ direction DIRECTION_LEFT amount 1 // 0@ = 0x000FFF02 
7011: 0@ = pack_rotate 0@ direction DIRECTION_RIGTH amount 2 // 0@ = 0xFF02000F

MODE_AND = 0
MODE_OR = 1
7012: 5@ = pack_check_truthy 0@ mask 0b1000 mode MODE_AND // IF 5@ = true
7012: 5@ = pack_check_truthy 0@ mask 0b0010 mode MODE_AND // IF 5@ = false
7012: 5@ = pack_check_truthy 0@ mask 0b1010 mode MODE_AND // IF 5@ = false
7012: 5@ = pack_check_truthy 0@ mask 0b1010 mode MODE_OR  // IF 5@ = true

//  0@ = 0x 00-04-08-0C
7013: 10@ = pack_swap_custom 0@ i3 4 i2 1 i1 3 i0 2
// 10@ = 0x 04-0C-08-00
```
A pack is actually an INT number. It's not a pointer to something weird.

## ⚖️ Ternary operator

The system includes a compact ternary operator for performing quick comparisons and returning a value depending on the result. It works just like the classic ternary operator in languages ​​such as C, JS, or C#, but adapted to the CLEO environment.

```js
OP_EQUAL = 0           // ==
OP_UNEQUAL = 1         // !=
OP_LESSER = 2          // <
OP_LESSER_EQUAL = 3    // <=
OP_GREATER = 4         // >
OP_GREATER_EQUAL = 5   // >=
```

The difference between each opcode lies mainly in the type of comparison that is made (INT, FLOAT or TRUTHY)

```js
700D: 0@ = 22 OP_EQUAL 21 ? 0xfff : 3.14159 // int op (0@ = 3.14159)
700E: 1@ = 50.0 OP_GREATER 2.33  ? 5.4 : 55@ // float op (0@ = 5.4)
7014: 2@ = 31@ ? 1.2 : 123 // if 31@ <> 0 then 2@ = 1.2 else 2@ = 123
```

However, assignments are treated as uint32 to ensure that what we want to pass is the exact same value.

## Widgets

Working with widgets has never been easier than this.
```js
7000: set_widget_transform 50 coords 250.0 100.0 scales 11.0 11.0
7001: get_widget_transform 50 coords 0@ 1@ scales 2@ 3@
```

## Lerp Movement

These commands are used to move a point from initial coordinates to final coordinates at a constant speed, using `deltaTime` to ensure smooth animation that is dependent on the frame rate.

* Returns interpolated coordinates
* Progress between `0.0` and `1.0`

### `move_lerp`

Perform an interpolation from one point to another, increasing the progress from `0.0 → 1.0` according to speed and `deltaTime`.
When it reaches `1.0`: it stops.

```js
0@ = 0.0
1@ = 0.0
2@ = 0.0

repeat
  wait 0
  4@ = 0.0
  0079: 4@ += frame_delta_time * 1.0 // (float)

  7020: 0@ 1@ 2@ progress 3@ = move_lerp 0@ 1@ 2@ to 10.0 10.0 10.0 deltatime 4@ speed 1.0

  Object.SetPosition($obj, 0@, 1@, 2@)
until 3@ <= 1.0
```

You must always pass the new coordinates within the opcode for the animation to be performed.

### `move_lerp_continuous`

Unlike the previous command, this one first reads the progress, then applies the calculations, and rewrites the progress variable.

```js
3@ = 0.0
repeat
  wait 0
  4@ = 0.0
  0079: 4@ += frame_delta_time * 1.0 // (float)

  7021: 0@ 1@ 2@ progress 3@ = move_lerp_continuous 0.0 0.0 0.0 to 10.0 10.0 10.0 deltatime 4@ speed 1.0

  Object.SetPosition($obj, 0@, 1@, 2@)
until 3@ <= 1.0
```

Here, the progress variable must always be defined from the outset with a value between `0.0` and `1.0`. If you enter a number such as `0.5`, you will obtain a number halfway between the initial and final values.

### `move_lerp_continuous_loop`

This version does not stop; once it reaches the end, it returns to the starting point.

```js
while true
  wait 0
  4@ = 0.0
  0079: 4@ += frame_delta_time * 1.0 // (float)

  7022: 0@ 1@ 2@ progress 3@ = move_lerp_continuous_loop 0.0 0.0 0.0 to 10.0 10.0 10.0 deltatime 4@ speed 1.0

  Object.SetPosition($obj, 0@, 1@, 2@)
end
```

For a value, use
```js
7023: 0@ progress 1@ = value_lerp 0.0 to 10.0 deltatime 8@ speed 1.0
7024: 0@ progress 1@ = value_lerp_continuous 0.0 to 10.0 deltatime 8@ speed 1.0
7025: 0@ progress 1@ = value_lerp_continuous_loop 0.0 to 10.0 deltatime 8@ speed 1.0
```

## File system

Renaming files, nothing more to say.
```js
7003: file_rename "DYOM.scm" to "ASS.scm"
```

If we need to manage a PATH but don't know whether it exists or not, we can use this opcode.
It is used to create files or folders depending on what we enter.
If they already exist, they are not replaced; they are only created if they do not exist.
```js
7004: create_file_or_directory "pop" // create file
7004: create_file_or_directory "pop.bin" // create file
7004: create_file_or_directory "/pop.bin" // create file
7004: create_file_or_directory "pop.bin/" // create folder
7004: create_file_or_directory "/pop.bin/" // create folder
```

## Others

Find the difference between the closest distances between two angles in degrees.
The results remain within a range of 180 to -180 degrees.
```js
7005: 0@ = angle_diff 0.0 10.0  // 10.0
7005: 0@ = angle_diff 50.0 60.0 // 10.0
7005: 0@ = angle_diff 350.0 0.0 // 10.0
7005: 0@ = angle_diff 0.0 350.0 // -10.0
7005: 0@ = angle_diff 0.0 190.0 // -170.0
7005: 0@ = angle_diff 190.0 0   // 170.0
```

These are just operations that should have been added in the early stages of CSM.
```js
701C: 0@ = !! 6.67  // truthy : 6.67 = true
701C: 0@ = !! 6     // truthy : 6    = true
701C: 0@ = !! 1     // truthy : 1    = true
701C: 0@ = !! 0     // falsy  : 0    = false

7006: 0@ = !  6.67  // falsy  : 6.67 = false
7006: 0@ = !  6     // falsy  : 6    = false
7006: 0@ = !  1     // falsy  : 1    = false
7006: 0@ = !  0     // truthy : 0    = true

7002: 0@ = 0 || 22 // 0@ = 22
7002: 0@ = 0 || 3.3 // 0@ = 3.3
7002: 0@ = 44 || 3.3 // 0@ = 44

7007: 0@ = 1@ / 1.22    // float
7008: 0@ = 2.2 * 3.14   // float
7009: 0@ = $x + MATH_PI // float
700A: 0@ = &0 - 22.2    // float
```

Divide a floating point number from where the decimal point is and choose how many decimal places you want to recover.
```js
700B: 0@ 1@= split_float_to_signed_parts 3.14159 decimals 4
// 0@ = 3
// 1@ = 1415 (4 decimals)
```

Convert a color model to another between RGB-HSL-HSV.
The function only accepts integers.
```js
COLOR_RGB_TO_HSV = 0
COLOR_RGB_TO_HSL = 1
COLOR_HSL_TO_HSV = 2
COLOR_HSL_TO_RGB = 3
COLOR_HSV_TO_HSL = 4
COLOR_HSV_TO_RGB = 5

700C: 0@ 1@ 2@ = convert_model_color COLOR_RGB_TO_HSL inputs 0 100 100
// 0@ = 255
// 0@ = 0
// 0@ = 0
```