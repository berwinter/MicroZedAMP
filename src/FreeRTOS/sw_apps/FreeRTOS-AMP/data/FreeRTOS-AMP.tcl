proc swapp_get_name {} {
    return "FreeRTOS AMP"
}

proc swapp_get_description {} {
    return "FreeRTOS AMP histogram demo for the second ARM core";
}

proc swapp_is_supported_hw {} {
    # check processor type
    set proc_instance [xget_processor_name];
    set proc_type [xget_ip_attribute "type" $proc_instance];

    if { $proc_instance != "ps7_cortexa9_1" } {
                error "This application is supported only for the second CortexA9 processors";
    }

    return 1;
}

proc swapp_is_supported_sw {} {
    # check for stdout being set
}

# depending on the type of os (standalone|xilkernel), choose
# the correct source files
proc swapp_generate {} {
}

proc swapp_get_linker_constraints {} {
    # don't generate a linker script. fsbl has its own linker script
    return "lscript no";
}

