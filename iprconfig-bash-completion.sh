#  IBM IPR adapter configuration utility
#
#  (C) Copyright 2000, 2015
#  International Business Machines Corporation and others.
#  All Rights Reserved. This program and the accompanying
#  materials are made available under the terms of the
#  Common Public License v1.0 which accompanies this distribution.

_comp_iprconfig () {
    iprconfig="${COMP_WORDS[0]}"
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    case "${prev}" in
	"-c")
	    opts=$(${iprconfig} -l 2>/dev/null)
	    COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
	    ;;
	"-k")
	    COMPREPLY=( $(compgen -o dirnames -- ${cur}) )
	    ;;
	*)
	    opts=$(find /dev -printf "%f\n" | grep -G "^\(sd\|sg\)")
	    COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
	    ;;
    esac
    return 0;
}
complete -F _comp_iprconfig iprconfig
