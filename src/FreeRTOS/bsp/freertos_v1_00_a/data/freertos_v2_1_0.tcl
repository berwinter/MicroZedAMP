##############################################################################
#
# (c) Copyright 2011 Xilinx, Inc. All rights reserved.
#
# This file contains confidential and proprietary information of Xilinx, Inc.
# and is protected under U.S. and international copyright and other
# intellectual property laws.
#
# DISCLAIMER
# This disclaimer is not a license and does not grant any rights to the
# materials distributed herewith. Except as otherwise provided in a valid
# license issued to you by Xilinx, and to the maximum extent permitted by
# applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
# FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
# IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
# MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
# and (2) Xilinx shall not be liable (whether in contract or tort, including
# negligence, or under any other theory of liability) for any loss or damage
# of any kind or nature related to, arising under or in connection with these
# materials, including for any direct, or any indirect, special, incidental,
# or consequential loss or damage (including loss of data, profits, goodwill,
# or any type of loss or damage suffered as a result of any action brought by
# a third party) even if such damage or loss was reasonably foreseeable or
# Xilinx had been advised of the possibility of the same.
#
# CRITICAL APPLICATIONS
# Xilinx products are not designed or intended to be fail-safe, or for use in
# any application requiring fail-safe performance, such as life-support or
# safety devices or systems, Class III medical devices, nuclear facilities,
# applications related to the deployment of airbags, or any other applications
# that could lead to death, personal injury, or severe property or
# environmental damage (individually and collectively, "Critical
# Applications"). Customer assumes the sole risk and liability of any use of
# Xilinx products in Critical Applications, subject only to applicable laws
# and regulations governing limitations on product liability.
#
# THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
# AT ALL TIMES.
#
###############################################################################

# standalone bsp version. set this to the latest "ACTIVE" version.
set standalone_version standalone_v3_05_a

proc FreeRTOS_drc {os_handle} {

    global env

    set sw_proc_handle [xget_libgen_proc_handle]
    set hw_proc_handle [xget_handle $sw_proc_handle "IPINST"]
    set proctype [xget_value $hw_proc_handle "OPTION" "IPNAME"]

}

proc generate {os_handle} {

    variable standalone_version

    set sw_proc_handle [xget_libgen_proc_handle]
    set hw_proc_handle [xget_handle $sw_proc_handle "IPINST"]
    set proctype [xget_value $hw_proc_handle "OPTION" "IPNAME"]
    
    set need_config_file "false"

    set armsrcdir "../${standalone_version}/src/cortexa9"
    set ccdir "../${standalone_version}/src/cortexa9/gcc"
	set commonsrcdir "../${standalone_version}/src/common"

	foreach entry [glob -nocomplain [file join $commonsrcdir *]] {
		file copy -force $entry [file join ".." "${standalone_version}" "src"]
	}
	
	# proctype should be "ps7_cortexa9"
	switch $proctype {
	
	"ps7_cortexa9"  {
		foreach entry [glob -nocomplain [file join $armsrcdir *]] {
			file copy -force $entry [file join ".." "${standalone_version}" "src"]
		}
		
		foreach entry [glob -nocomplain [file join $ccdir *]] {
					file copy -force $entry [file join ".." "${standalone_version}" "src"]
		}
		
		set need_config_file "true"
		
		set file_handle [xopen_include_file "xparameters.h"]
		puts $file_handle "#include \"xparameters_ps.h\""
		puts $file_handle ""
		close $file_handle
		}
		"default" {puts "processor type $proctype not supported\n"}
	}
	
	# Write the Config.make file
	set makeconfig [open "../${standalone_version}/src/config.make" w]
	
	if { $proctype == "ps7_cortexa9" } {
	    puts $makeconfig "LIBSOURCES = *.c *.s *.S"
	    puts $makeconfig "LIBS = standalone_libs"
	}
	
	close $makeconfig

	# Remove arm directory...
	file delete -force $armsrcdir

	# copy required files to the main src directory
	file copy -force [file join src Source tasks.c] ./src
	file copy -force [file join src Source queue.c] ./src
	file copy -force [file join src Source list.c] ./src
	file copy -force [file join src Source timers.c] ./src
	file copy -force [file join src Source portable MemMang heap_3.c] ./src
	file copy -force [file join src Source portable GCC Zynq port.c] ./src
	file copy -force [file join src Source portable GCC Zynq portISR.c] ./src
	file copy -force [file join src Source portable GCC Zynq port_asm_vectors.s] ./src
	file copy -force [file join src Source portable GCC Zynq portmacro.h] ./src
	
	set headers [glob -join ./src/Source/include *.\[h\]]
	foreach header $headers {
		file copy -force $header src
	}
	
	file delete -force [file join src Source]
	file delete -force [file join src Source]

	# Handle stdin and stdout
	xhandle_stdin $os_handle
	xhandle_stdout $os_handle

# ToDO: FreeRTOS does not handle the following, refer xilkernel TCL script
# - MPU settings

    set config_file [xopen_new_include_file "./src/FreeRTOSConfig.h" "FreeRTOS Configuration parameters"]
    puts $config_file "\#include \"xparameters.h\" \n"

    set val [xget_value $os_handle "PARAMETER" "use_preemption"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_PREEMPTION" "0"
    } else {
        xput_define $config_file "configUSE_PREEMPTION" "1"
    }

    set val [xget_value $os_handle "PARAMETER" "use_mutexes"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_MUTEXES" "0"
    } else {
        xput_define $config_file "configUSE_MUTEXES" "1"
    }
    
    set val [xget_value $os_handle "PARAMETER" "use_recursive_mutexes"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_RECURSIVE_MUTEXES" "0"
    } else {
        xput_define $config_file "configUSE_RECURSIVE_MUTEXES" "1"
    }

    set val [xget_value $os_handle "PARAMETER" "use_counting_semaphores"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_COUNTING_SEMAPHORES" "0"
    } else {
        xput_define $config_file "configUSE_COUNTING_SEMAPHORES" "1"
    }

    set val [xget_value $os_handle "PARAMETER" "use_timers"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_TIMERS" "0"
    } else {
        xput_define $config_file "configUSE_TIMERS" "1"
    }

    set val [xget_value $os_handle "PARAMETER" "use_idle_hook"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_IDLE_HOOK"    "0"
    } else {
        xput_define $config_file "configUSE_IDLE_HOOK"    "1"
    }

    set val [xget_value $os_handle "PARAMETER" "use_tick_hook"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_TICK_HOOK"    "0"
    } else {
        xput_define $config_file "configUSE_TICK_HOOK"    "1"
    }

    set val [xget_value $os_handle "PARAMETER" "use_malloc_failed_hook"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_MALLOC_FAILED_HOOK"    "0"
    } else {
        xput_define $config_file "configUSE_MALLOC_FAILED_HOOK"    "1"
    }

    set val [xget_value $os_handle "PARAMETER" "use_trace_facility"]
    if {$val == "false"} {
        xput_define $config_file "configUSE_TRACE_FACILITY" "0"
    } else {
        xput_define $config_file "configUSE_TRACE_FACILITY" "1"
    }

    xput_define $config_file "configUSE_16_BIT_TICKS"   "0"
    xput_define $config_file "configUSE_APPLICATION_TASK_TAG"   "0"
    xput_define $config_file "configUSE_CO_ROUTINES"    "0"

    #set systmr_interval [xget_value $os_handle "PARAMETER" "systmr_interval"]
    xput_define $config_file "configTICK_RATE_HZ"     "( ( portTickType ) 100 )"
    
    set max_priorities [xget_value $os_handle "PARAMETER" "max_priorities"]
    xput_define $config_file "configMAX_PRIORITIES"   "( ( unsigned portBASE_TYPE ) $max_priorities)"
    xput_define $config_file "configMAX_CO_ROUTINE_PRIORITIES" "2"
    
    set min_stack [xget_value $os_handle "PARAMETER" "minimal_stack_size"]
    set min_stack [expr [expr $min_stack + 3] & 0xFFFFFFFC]
    xput_define $config_file "configMINIMAL_STACK_SIZE" "( ( unsigned short ) $min_stack)"

    set total_heap_size [xget_value $os_handle "PARAMETER" "total_heap_size"]
    set total_heap_size [expr [expr $total_heap_size + 3] & 0xFFFFFFFC]
    xput_define $config_file "configTOTAL_HEAP_SIZE"  "( ( size_t ) ( $total_heap_size ) )"

    set max_task_name_len [xget_value $os_handle "PARAMETER" "max_task_name_len"]
    xput_define $config_file "configMAX_TASK_NAME_LEN"  $max_task_name_len
    
    set val [xget_value $os_handle "PARAMETER" "idle_yield"]
    if {$val == "false"} {
        xput_define $config_file "configIDLE_SHOULD_YIELD"  "0"
    } else {
        xput_define $config_file "configIDLE_SHOULD_YIELD"  "1"
    }
    
    set val [xget_value $os_handle "PARAMETER" "timer_task_priority"]
	if {$val == "false"} {
		xput_define $config_file "configTIMER_TASK_PRIORITY"  "0"
	} else {
		xput_define $config_file "configTIMER_TASK_PRIORITY"  "10"
	}

	set val [xget_value $os_handle "PARAMETER" "timer_command_queue_length"]
	if {$val == "false"} {
		xput_define $config_file "configTIMER_QUEUE_LENGTH"  "0"
	} else {
		xput_define $config_file "configTIMER_QUEUE_LENGTH"  "10"
	}

	set val [xget_value $os_handle "PARAMETER" "timer_task_stack_depth"]
	if {$val == "false"} {
		xput_define $config_file "configTIMER_TASK_STACK_DEPTH"  "0"
	} else {
		xput_define $config_file "configTIMER_TASK_STACK_DEPTH"  $min_stack
	}
	
    xput_define $config_file "INCLUDE_vTaskCleanUpResources" "0"
    xput_define $config_file "INCLUDE_vTaskDelay"        "1"
    xput_define $config_file "INCLUDE_vTaskDelayUntil"   "1"
    xput_define $config_file "INCLUDE_vTaskDelete"       "1"
    xput_define $config_file "INCLUDE_uxTaskPriorityGet" "1"
    xput_define $config_file "INCLUDE_vTaskPrioritySet"  "1"
    xput_define $config_file "INCLUDE_vTaskSuspend"      "1"


    # complete the header protectors
    puts $config_file "\#endif"
    close $config_file
}

proc xopen_new_include_file { filename description } {
    set inc_file [open $filename w]
    xprint_generated_header $inc_file $description
    set newfname [string map {. _} [lindex [split $filename {\/}] end]]
    puts $inc_file "\#ifndef _[string toupper $newfname]"
    puts $inc_file "\#define _[string toupper $newfname]\n\n"
    return $inc_file
}

proc xput_define { config_file parameter param_value } {
    puts $config_file "#define $parameter $param_value\n"

    # puts "creating #define [string toupper $parameter] $param_value\n"
}
