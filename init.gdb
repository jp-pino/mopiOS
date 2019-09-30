define ptcb
  set var $n = $arg0
  if $n
    printf "---------------------\n"
    printf "NAME:      %s\n\n", $n->name
    printf "TCB:       %p\n", $n
    printf "SP:        %p\n", $n->sp
    printf "NEXT:      %p\n", $n->next
    printf "PREV:      %p\n", $n->prev
    printf "ID:        %i\n", $n->id
    printf "STK_S:     %i\n", $n->stackSize
    printf "USE:       %i\n", $n->inUse
    printf "SLEEP:     %i\n", $n->sleepTime
    printf "PRI:       %i\n", $n->priority
    printf "DEL:       %i\n", $n->deleteThis
    printf "BLCKD:     %i\n", $n->isBlocked
    printf "STCK:      %p\n", $n->stack
    printf "PARNT:     %p\n", $n->parent
    printf "---------------------\n"
  end
end

define psema
  set var $n = $arg0
  if $n
    printf "---------------------\n"
    printf "NAME:      %s\n\n", $n->name
    printf "SEMA:      %p\n", $n
    printf "NEXT:      %p\n", $n->next
    printf "BLOCKED:   %p\n", $n->blocked
    printf "WHO:       %i\n", $n->who
    printf "VAL:       %d\n", $n->value
    printf "---------------------\n"
  end
end

define semas
  set var $n = SemaPt
  while $n
    psema $n
    set var $n = $n->next
  end
end


define plist
  set var $last = 0
  set var $curr = $arg0
  if $curr
    while $last != $arg0->prev
      set var $last = $curr
      ptcb $curr
      set var $curr = $curr->next
    end
  end
end

define tcbs
  set var $l = 8
  set var $c = 0
  while $c < $l
    printf "\n\n---------------------"
    printf "\nPriority %i", $c
    printf "\n---------------------\n"
    set var $last = 0
    set var $curr = tcbLists[$c]
      if $curr
        while $last != tcbLists[$c].prev
          set var $last = $curr
          ptcb $curr
          set var $curr = $curr->next
        end
      end
    set var $c = $c + 1
  end
end

define perror
  printf "\n---------------------\n"
  printf "R0:    %#010x\n", r0
  printf "R1:    %#010x\n", r1
  printf "R2:    %#010x\n", r2
  printf "R3:    %#010x\n", r3
  printf "R12:   %#010x\n", r12
  printf "LR:    %#010x\n", lr
  printf "PC:    %#010x\n", pc
  printf "PSR:   %#010x\n", psr
  printf "BFAR:  %#010x\n", bfar
  printf "CFSR:  %#010x\n", cfsr
  printf "HFSR:  %#010x\n", hfsr
  printf "DFSR:  %#010x\n", dfsr
  printf "AFSR:  %#010x\n", afsr
end

define semalist
	set var $n = SemaPt
	while $n
		printf "%s\n", $n->name
		set var $n = $n->next
	end
end

define testsema
	set var $n = SemaPt
	while $n
		set var $n = $n->next
	end
	printf "OK\n\n"
end


target extended-remote :3333

load
