## 교육용 운영체제 xv6에 새로운 기능을 구현하기
assignment : implementing a system call

project01 : implementing system schedulers
project02 : 
project03 :
<br>

## 실행환경
Ubuntu-24.04
QEMU 하이퍼바이저 - 하드웨어 가상화

---
# xv6 개념정리
`xv6 book` : https://pdos.csail.mit.edu/6.S081/2020/xv6/book-riscv-rev1.pdf
`xv6 homework` : https://pdos.csail.mit.edu/6.828/2018/homework
`xv6 소스코드` : git://github.com/mit-pdos/xv6-public.git
`xv6 printout` : https://pdos.csail.mit.edu/6.828/2018/xv6/xv6-rev10.pdf
(또는 ctags+cscope로 코드분석 가능)

`git book` https://xiayingp.gitbook.io/build_a_os/hardware-device-assembly/start-xv6-and-the-first-process
코드 설명 https://github.com/palladian1/xv6-annotated
https://xiayingp.gitbook.io/build_a_os/hardware-device-assembly/why-first-user-process-loads-another-program

## xv6

> > Unix V6을 기반으로 만들어진, 간단한 기능만을 포함한 운영체제이다.

- `모놀리식 커널` 전체 운영체제가 커널 내에 존재하여 모든 시스템 콜의 구현이 커널 모드에서 실행되도록 하는 것
- 6000줄 코드(c언어 대부분, 어셈블리어 300줄)
- RISC, x86 (64bit)
- 32개의 register + pc

  - `zero`
  - `ra` : 함수가 call 되면 (스택 대신) ra에 저장, return 명령어는 ra에 저장된 값을 pc로 복사 -> 메모리에 접근할 필요가 없다!
  - `sp` stack pointer
  - `tp` thread pointer (코어 번호) - 커널 코드에선 바뀌지 않는다.
    - kernel이 user code로 갈 때, 모든 register를 저장한다. (이 때 user code는 tp register의 값을 변경시킬 수 있다!) 그리고 다시 kernel mode로 돌아올 때, 모든 register를 다시 복원한다.
  - `gp` global pointer (complier가 사용하며 바뀌지 않음)
  - `a0`~`a7` 함수에게 argument 전달
  - `t0`~`t6`
  - `s0`~`s11` callee-saved register
    - 함수는 이 레지스터를 사용하기 전에 스택에 저장하고, return하기 전에 복원해야 한다.

- RISC-5 모드
  - machine mode
    - startup+initialization, timer interrupt
  - supervisor mode
  - user mode
    - user mode에서 수행 불가능한 instruction은 trap+abort를 발생시킨다.

<br>

### 전형적인 OS와 비교했을 때 없는 것

user IDs, login
file protection
'mount'able file systems (just one file system)
paging virtual address spaces to disk
sockets, support for networks
interprocess communication
2 device drivers

## 특징

- 운영체제의 여러 부분이 모듈화 되어있어 이해하기 쉽고 수정이 용이하다.
  ex) `proc.c` : 프로세스 관리 모듈
  파일 시스템 모듈
  메모리 관리 모듈

- SMP(Shared Memory Multiprocessor) :

  - 멀티코어
  - 메인메모리(128MB)를 공유한다. (커널 코드에 #define으로 hardwired 되어있음)

- 한 프로세스가 다른 프로세스의 메모리,cpu,파일 등을 읽거나 쓸 수 없다.
- 프로세스의 스레드는 사용자 스택(사용자코드)과 커널 스택(커널 코드)을 번갈아가며 사용한다.

## devices

`UART` keyboard로부터 printing, reading input을 받는 통신 채널 제공 (byte stream)
`DISK` 컴퓨터 디스크를 따라가는 1개의 디스크 존재
(UART와 DISK는 코어끼리 공유)
`Timer interrupt` 각각의 코어가 가짐.(local)
`PLIC(Platform Level Interrupt Controller)`각종 device로부터의 interrupt를 관리. (어떤 코어가 interrupted 됐는지 등)

- `Page table`
  - 1 page table / kernel (모든 core가 공유),
  - 1 table / process
- 3 level page table
- Page size = 4KB (fixed)
- virtual address size = 256GB(2^38bit)

- `single free list`(사용되지 않은 pages) : 메모리가 필요하면 여기서 할당되거나 다시 반환됨
- No objects, malloc

## Scheduler

`Round Robin`  
:각 프로세스의 timeslice의 크기는 고정 (1,000,000 cycles)
모든 코어는 1개의 Ready Queue(ready list, run list)를 공유
코어가 ready queue에서 다음 실행할 process를 결정. (time slice가 끝나면 ready queue로 return)

- 각 코어가 반복적으로 ready queue를 탐색하며 실행 가능한 process 찾음.
  <br>

## Boot sequence

`QEMU` loads kernel code at fixed address, starts all cores running.

## Locking

`Spin locks`
(0 : lock is free, 1 : lock is busy)

1. acquire : lock이 0이 될 때까지 기다리고, 1로 바꿈
2. release : lock을 0으로 바꿈

`sleep()` 특정 스레드가 sleep 함수를 사용하면, timeslice를 끝내고 ready queue에 not runnable 상태로 돌아옴.
`wakeup()` 특정 스레드가 wakeup 함수를 사용하면, sleep/block 상태에서 runnable상태로 바뀜.

interrupt를 선택적으로 불가능하게 하여 코어에서 실행 중인 스레드가 방해 받지 않음.

각 코어는 status control word를 가짐.

## User Address Space

(그림)
kernel이 virtual address space를 만들고, stack의 page에 argv, argc 등의 argument를 push.

user program이 실행되면 이 argument들을 stack에서 찾을 수 있다.

## xv6 파일 구조

- kernel

  - `bio.c` Disk block cache for the file system.
  - `console.c` Connect to the user keyboard and screen.
  - `entry.S` Very first boot instructions.
  - `exec.c` exec() system call.
  - `file.c` File descriptor support.
  - `fs.c` File system 관리
  - `kalloc.c` Physical page allocator.
  - `kernelvec.S` Handle traps from kernel, and timer interrupts.
  - `log.c` File system logging and crash recovery.
  - `main.c` Control initialization of other modules during boot.
  - `pipe.c` Pipes.
  - `plic.c` RISC-V interrupt controller.
  - `printf.c` Formatted output to the console.
  - `proc.c` Processes and scheduling.
  - `sleeplock.c` Locks that yield the CPU.
  - `spinlock.c` Locks that don’t yield the CPU.
  - `start.c` Early machine-mode boot code.
  - `string.c` C string and byte-array library.
  - `swtch.S` Thread switching.
  - `syscall.c` 시스템콜을 핸들러로 전달
  - `sysfile.c` File-related system calls.
  - `sysproc.c` Process-related system calls.
  - `trampoline.S` Assembly code to switch between user and kernel.
  - `trap.c` C code to handle and return from traps and interrupts.
  - `uart.c` Serial-port console device driver.
  - `virtio_disk.c` Disk device driver.
  - `vm.c` Manage page tables and address spaces.(가상 메모리)

- user

  - `initcode.S` user mode에서 가장 먼저 실행되는 프로세스.
  - `sh.c` 사용자가 명령을 입력하고 실행하는 shell 프로그램의 코드
  - `cat.c`
  - `echo.c`
  - `grep.c`
  - `kill.c`
  - `ln.c` : create a hard link between files
  - `ls.c`
  - `mkdir.c`
  - `rm.c`
  - `wc.c` : count the words/characters in a file
  - `usys.S` 시스템콜을 위해 user mode에서 kernel mode로 전환하는 어셈블리 코드. 시스템콜 번호와 인수 전달

- both

  - `types.h` 데이터 타입을 정의
  - `param.h` 시스템의 여러 매개변수, 상수를 정의
    - fixted limits
      - #define number of processes
      - ready queue(array)는 고정된 크기를 가짐.
      - #define number of 동시open files
    - Linked list 대신 주로 `배열`을 사용한다.
      - ex) `kill(pid)` process배열에서 선형 탐색함
  - `defs.h` xv6에서 공통적으로 사용되는 상수,매크로,정의 등을 모아둠
  - `syscall.h` 시스템콜 번호 지정
    - 프로세스 상태에 대한 정의(UNUSED, EMBRYO, RUNNABLE, RUNNING, SLEEPING, ZOMBIE 등)
  - `user.h` 시스템콜 함수과 library함수의 프로토타입을 모아둠

- etc
  - Makefile
  - README
  - LICENSE

## 실행 flow

xv6를 실행하면 모든 코어는 동시에 아래 파일을 실행한다.
`entry.S` 각 코어마다 set up stack - sp, tp(thread pointer) register (number of core)
`start.c` machine code (일부 커널)
`main.c` supervisor mode (거의 모든 커널)

## 함수

`cpuid()` 는 프로세서가 멀티코어와 하이퍼스레딩을 지원하는지, L1,L2,L3캐시의 크기와 구성 등의 정보를 확인한다.
현재 실행 중인 cpu(코어)의 id를 반환한다.(첫 번째 cpu는 0)

스레드가 커널 모드로 전환될 때 사용하는 tp register를 반환한다.

`np` fork()함수 내에서 새로 생성되는 자식 프로세스를 가리키는 포인터
`mhartid` 코어 번호를 저장하는 레지스터. 시작 시 `tp`레지스터로 이동

`volatile static int started = 0;` 여러 코어 간 동기화를 위한 코드
`p->xxx` proc 구조체의 요소에 접근할 때
`p->state` 프로세스가 할당된 상태인지, 실행 준비가 된 상태인지, 실행 중인 상태인지, I/O를 기다리는 상태인지, 또는 종료 상태인지를 나타낸다.

- UNUSED:
  아직 프로세스가 생성되지 않았거나, 종료된 프로세스의 자원이 아직 해제되지 않았을 때

- EMBRYO:
  EMBRYO 상태의 프로세스는 생성되었지만 초기화되지 않은 상태입니다. 프로세스가 생성되고 초기화되는 동안 해당 상태가 설정됩니다. 초기화가 완료되면 EMBRYO 상태에서 다른 상태로 전이됩니다.

- SLEEPING:
  SLEEPING 상태의 프로세스는 특정 이벤트가 발생할 때까지 대기하고 있는 상태입니다. 예를 들어, 파일 I/O 완료를 기다리는 경우나 특정 시간이 지날 때까지 대기하는 경우에 해당합니다.

- RUNNABLE:
  RUNNABLE 상태의 프로세스는 실행 가능한 상태이며, CPU를 할당받을 준비가 된 상태입니다. 즉, 다른 프로세스가 CPU를 사용하고 있지 않을 때 실행될 수 있는 상태입니다.

- RUNNING:
  RUNNING 상태의 프로세스는 현재 CPU를 사용하고 실행 중인 상태입니다. CPU를 할당받아 코드를 실행하고 있는 상태를 나타냅니다.

- ZOMBIE:
  ZOMBIE 상태의 프로세스는 종료된 상태이지만, 부모 프로세스가 종료 상태를 확인하기 위해 대기 중인 상태입니다. 이러한 상태는 프로세스의 자원이 완전히 해제되기 전까지 유지됩니다.

`p->pgdir`는 프로세스의 페이지 테이블을 x86 하드웨어가 예상하는 형식으로 보유
`p->kstack` 커널 스택
`p->pagetable` holds the process’s page table, in the format that the RISC-V hardware expects.
(pagetable은 하드웨어에 구현되어 있다.)
