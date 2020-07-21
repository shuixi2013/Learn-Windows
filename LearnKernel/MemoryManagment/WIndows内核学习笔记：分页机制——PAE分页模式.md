# 前言
物理地址扩展 Physical Address Extension（缩写**PAE**）。
解释：正常情况32位CPU可以存取4G的物理内存，但在现实中，实际情况是内存的发展速度大于CPU的发展速度，所以才有了让32位CPU存取超过4G内存的需求。实现方式就是给CPU增加了4根地址线，达到36根，于是可寻址64G大小的内存。

Intel到目前为止设计了4种分页模式，分别是：32位、PAE、4-level、5-level这四种模式。本文主要讲了Intel是如何设计PAE模式的页表相关数据结构。
 - “Chapter 4 Paging”部分，是笔者在阅读Intel手册时做的翻译笔记，对应原文的 *Volume3: Chapter 4 Paging*。翻译有不准确的地方，以原文为准。可以在这里下载手册。[Intel® 64 and IA-32 Architectures Software Developer Manuals](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html) 
 - “实验题”部分，实现了一个可以浏览进程页表的小工具，可以在这里查看源码。
<br>

# Chapter 4 Paging
## 4.1 分页模式和控制位
分页控制有关的寄存器
- CR0    标志位：WP（bit 16）、PG（bit 31）
- CR4    标志位：PSE（bit 4）、PAE（bit 5）、PGE（bit 7）、PCIDE（bit 17）、SMEP（bit 20）、SMAP（bit 21）、PKE（bit 22）、CET（bit 23）、PKS（bit 24）
-  IA32_EFER MSR    标志位：LEM（bit 8）、NXE（bit 11)
- EFLAGS    标志位：AC（bit 18）

软件（应该指操作系统）如何启用分页功能：确保CR3中是分页结构表的物理内存地址，然后使用MOV指令置CR0.PG位。

<br>

### 4.1.1 四种分页模式
本节内容：根据 CR0.PG、CR4.PAE、CR4.LA57和IA32_EFER.LME，判断是否启用分页，以及是开启的什么分页模式。

**CR0.PG = 0**  表示未启用分页模式，此时会把线性地址直接当作物理地址使用。

 **CR0.PG = 1** 表示启用分页模式。 Paging can be enabled only if protection is enabled (CR0.PE = 1)。此时由CR4.PAE、CR4.LA57、和IA32_EFER.LME决定启用哪种分页模式。
 

 - **32-bit paging 模式**       CR4.PAE = 0 （详见4.3节）
 - **PAE paging 模式**         CR4.PAE = 1 and IA32_EFER.LME = 0    （详见4.4节）
 - **4-level paging 模式**     CR4.PAE = 1, IA32_EFER.LME = 1, and CR4.LA57 = 0    （4表示4层表寻找Pages页，详见4.5节）
 - **5-level paging 模式**     CR4.PAE = 1, IA32_EFER.LME = 1, and CR4.LA57 = 1   （5表示5层表寻找Pages页，详见4.5节）
32-bit 和PAG模式用于保护模式32位模式，IA32_EFER.LME = 0
4-level和5-level模式用于64位模式（IA-32e表示64位模式），IA32_EFER.LME = 1

四种模式的区别：

 - 线性地址宽度
 - 物理地址宽度
 - 分页大小：4K、2M、4M、1G
 - 支持execute-disable，数据执行保护
 - 支持PCIDs，操作系统可以启用缓存线性地址的功能，不是很懂，先放放
 - 支持 protection keys，不知道干嘛的
![四种分页模式的差异](https://img-blog.csdnimg.cn/20200717222807678.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl8zODUyNjE4MA==,size_16,color_FFFFFF,t_70)
**4-level** 和 **5-level**模式还有两个子模式：
 - 兼容模式：兼容32位的模式
 - 64位模式：虽然是64位线性地址，但是实际上4-level只使用了低48位做线性地址，而5-level页只使用低57位，物理地址线都只有52根。

<br>

### 4.1.2 启用和切换分页模式
本节内容：根据 CR0.PG、CR4.PAE、CR4.LA57和IA32_EFER.LME，如何启用并且切换不同的分页模式
讲解在不同模式之间切换的规则，这部分不感兴趣，略过。

<br>

### 4.1.3 分页属性控制
本节内容：通过CR0.WP、CR4.PSE、CR4.PGE、CR4.PCIDE、CR4.SMEP、CR4.SMAP、CR4.PKE、CR4.CET、CR4.PKS和IA32_EFER.NXE控制在不同分页模式下Pages的属性

 - **CR0.WP** 
 写入数据保护标志位：
等于0， supervisor-mode（应该指0环应用程序）可以向具有只读属性的页写数据；等于1，则不可以操作。
这个标志位对User-mode（应该时3环应用程序）没有影响，因为只要是只读属性的页，3环程序都不能写。
（4.6节有更详细的介绍）
- **CR4.PSE** 
是否启用4M分页：
只对**32-bit paging 模式**作用，等于0，表示分页大小只能是4K；等于1，可以选择4K或4M分页。其它三种模式的分页大小可以自由选择，不受该位的控制。（4.3节有更详细的介绍）
- **CR4.PGE**  
是否启用全局共享页：
等于0，不同进程间不会共享物理内存；等于1，进程间可以共享物理内存。（可能翻译的不对，帖上原文）
CR4.PGE enables global pages. If CR4.PGE = 0, no translations are shared across address spaces; if CR4.PGE = 1, 
specified translations may be shared across address spaces.（4.10.2.4节有更详细的介绍）
- **CR4.PCIDE**
启用process-context identifiers，对**4-level** 和 **5-level**模式作用。
PCIDs逻辑处理器缓存多个线性地址。（4.10.1节有更详细的介绍）
- **CR4.SMEP**
 If CR4.SMEP = 1, software operating in supervisor mode cannot fetch instructions from linear addresses that are accessible in user mode.（4.6节有更详细的介绍）
- **CR4.SMAP**
If CR4.SMAP = 1, software operating in supervisor mode cannot access data at linear addresses that are accessible in user mode. Software can override this protection by setting EFLAGS.AC.

- **CR4.PKE and CR4.PKS**
4-level和5-level模式将每一个线性地址与保护key相关联。
CR4.PKE=1时，PKRU寄存器表示，user-mode的线性地址所关联的保护key，是否可读或可写。
CR4.PKS=1时， the IA32_PKRS MSR does the same for supervisor-mode linear addresses.

- **CR4.CET**
这个好难理解。
If CR4.CET = 1, certain memory accesses are identified as shadow-stack accesses and certain linear addresses translate to 
shadow-stack pages.

- **IA32_EFER.NXE**
执行保护，对**4-level** 和 **5-level**模式作用。如果设为1，则不能执行指令，但是可以读指令。


<br>

### 4.1.4 Enumeration of Paging Features by CPUID
这部分保护标志位的意义和用法。
PSE: page-size extensions for 32-bit paging.
PAE: physical-address extension.
PGE: global-page support.
PAT: page-attribute table.
PSE-36: page-size extensions with 40-bit physical-address extension.
PCID: process-context identifiers.
SMEP: supervisor-mode execution prevention.
SMAP: supervisor-mode access prevention.
PKU: protection keys for user-mode pages.
OSPKE: enabling of protection keys for user-mode pages.
CET: control-flow enforcement technology.
LA57: 57-bit linear addresses and 5-level paging.
PKS: protection keys for supervisor-mode pages.
NX: execute disable.
Page1GB: 1-GByte pages.
LM: IA-32e mode support.
CPUID.80000008H:EAX[7:0] reports the physical-address width supported by the processor.
CPUID.80000008H:EAX[15:8] reports the linear-address width supported by the processor.

<br>

## 4.2 分层页表结构概览
不同模式使用的页表结构是不一样的，有的只是用2张表，有的则更多。
每张表的大小都是4096字节，对于32-bit模式，每一项是4字节，共1024项；对于其它三种模式，每一项是8字节，每张表512项。PAE模式是个特例，它的第一张表只有4项。

不同模式对线性地址的处理是不一样的，详细章节会讲。

这里提出了两个名词：**page frame** 线性地址中用来寻址的部分；**page offset** 线性地址中用作偏移的部分。

每一项中的地址部分都是物理内存地址。

第一张表总是保存在CR3寄存器中。

四种模式解析线性地址（4K分页举例）：
- 32-bit模式：32:22（10位）表1下标，21:12（10位）表2下标，11:0（12位）用作分页内的偏移量。
- PAE模式：31:30（2位）表1下标，29:21（9位）表2下标，20:12（9位）表3下标，11:0（12位）分页内的偏移量。
- 4-level模式：每张表都是512项，总共4张表，所以47:39、38:30、29:21、20:12对应4张表的下标，11:0（12位）分页内的偏移量。
- 5-level模式：使用5张表，56:48是第一张表下标，其余的根4-level模式的一样。

总结上面的解析线性地址过程，其实就是查表，查表，再查表。有些情况查表过程可能会中断，比如说遇到缺页异常时。
还有两种特殊的情况：
 - 查表过程中，剩余没有解析的线性地址宽度超过12位，如果当前表项的属性位bit 7（PS位—page size）等于1，当前项就是最后的页
 - 查表过程中，剩余没有解析的线性地址宽度等于12位，bit 7不再是PS位，另有它用，当前项则指向另一个表

对上述的第一种情况举例：
32-bit模式，如果分页大小是4M（CR4.PSE=1），那么表1就是存储的页，总共1024项，1024*4M=4G，正好寻址4G空间，表2就不存在了。再比如，PAE模式下，如果分页大小是2M，查到第2张表就时页了，此时没有了表3。

不同的表结构都有名字，参考下图：
![不同分页模式的分页结构体](https://img-blog.csdnimg.cn/20200717233806804.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl8zODUyNjE4MA==,size_16,color_FFFFFF,t_70)
对于上面的缩写，通常我们有下面的叫法：
PTE 页表，存放的每一项是最终的Pages物理地址，32位的分页大小有4K、2M、4M，64位则能扩展到1G大小
PDE 页目录表，存放的每一项是PTE
PDPTE页目录指针表，PAE模式只有4项，4-level和5-level模式都是填满的512项，每一项指向一个页目录表。
PML4E和PML5E暂时也不知道，实际上用法类似，各增加一层表的
<br>

## 4.4 PAE 分页模式
寄存器标志位：
CR0.PG = 1
CR4.PAE = 1
IA32_EFER.LME = 0

<br>

### 4.4.1 PDPTE寄存器
CR3指向 page-directory-pointer表。其中：
4:0	没有用
31:5	存放指向表的物理地址
63:52 没有用

page-dirctory-pointer表中由4个8字节的大小的PDPTEs组成，每个可寻址1-GByte大小的线性地址空间。
对应这4个PDPTE，逻辑处理器内部维护着与之对应的四个non-architectural寄存器，分别是PDPTE0、PDPTE1、PDPTE2、PDPTE3。出现下面几种情况时，逻辑处理器会重新加载内存中的PDPTEs到4个寄存器：

 - 如果使用MOV to CR0或MOV to CR4指令修改了这些寄存器中的标志位（CR0.CD, CR0.NW, CR0.PG, CR4.PAE, CR4.PGE, CR4.PSE, or CR4.SMEP），PDPTESs会从CR3中执行的地址重新加载
- 在PAE分页模式，如果下执行MOV to CR3指令，PDPTESs会从CR3中执行的地址重新加载
- 在PAE分页模式，如果CR3中的值被task switch修改，会从新的CR3中加载PDPTEs
下表中说明了PDPTE的结构：

| 位 | 用途 |
|--|--|
| 0（P） | Present，等于1表示此项指向一个页目录表，等于0则此项不包含页目录表 |
| 2:1 | 保留位，必须填0 |
|3（PWT）|Page-level write-through，用来间接确定访问页目录表所需要的内存属性（详见4.9节）|
|4（PCD）|Page-level cache disable，用来间接确定访问页目录表所需要的内存属性（详见4.9节）|
|8:5|保留位，必须填0|
|11:9|忽略|
|(M-1):12|指向页目录表的4K对齐的物理地址|
|63:M|保留位，必须填0|
注：M表示MAXPHYADDR，最大物理地址宽度，该手册中是52根地址线。

<br>

### 4.4.2 线性地址转物理地址
PAE模式下，可以使用4K或2M分页。
如何确定是4K分页还是2M分页：

 - 线性地址的31:30（2位）用来选择4个PDPTE寄存器中一个，称为*PDPTEi*，i等于这两位的值。每个*PDPTEi*可以对应1G大小的线性地址空间。如果*PDPTEi*的P位是0，那么这一项就是无效的，就是说该项不包含对应的页目录表，同时会产生一个page-fault异常（详见4.7节）。
 - 如果*PDPTEi*的P位是1，那么其51:12（40位）指向了页目录表的物理地址。每个页目录表由512项8字节的PDEs组成。

意思就是线性地址的前两位用于确定页目录指针表(PDPTE)的下标，找到4个中的一个页目录表。
页目录表，每个页面录表含有512个表项，每个表项8字节大小，指向一个页表或者指向一个2M的页，这要根据下面介绍的PS标志位来确定。

 - 如果PDE's页目录表项的PS位等于1，说明这一项指向的就是一个最终的2M的分页，物理地址由该项的51:21（31位）和20:0（21位）确定。物理地址线最大52根，有的CPU是36根，那么高位就是35:21（15位），有的CPU有52根的，高位才是51:21。
 - 如果PDE's页目录表项的PS位等于0，说明这一项指向的是页表。页表的物理地址跟上面的情况一样也分为36根物理地址线和52根物理地址线。
 
如果是2M分页，那么线性地址后30位中，29:21（9位）用来作为表2（页目录表）的下标索引，来确定最终的页地址；20:0（21位）作为2M页内的偏移量。
如果是4K分页，那么线性地址后30位中，29:21（9位）作为表2（页目录表）的下标索引，20:12（9位）作为表3（页表）的下标索引，11:0（12位)作为4K页内的偏移量。

如果页目录表项（PDE）或者页表项（PTE）的P位（bit 0）等于0，或者置位它们的任意一位保留位，这个表项将会失效，并且会引起page-fault异常（详见4.7节）。

PAE模式中的保留位有下面几个：

 - If the P flag (bit 0) of a PDE or a PTE is 1, bits 62:MAXPHYADDR are reserved.
 - If the P flag and the PS flag (bit 7) of a PDE are both 1, bits 20:13 are reserved.
 - If IA32_EFER.NXE = 0 and the P flag of a PDE or a PTE is 1, the XD flag (bit 63) is reserved
 - If the PAT is not supported:
— If the P flag of a PTE is 1, bit 7 is reserved.
— If the P flag and the PS flag of a PDE are both 1, bit 12 is reserved.

下图是4K分页的线性地址转物理地址的过程示意图。
![4K分页的地址转换](https://img-blog.csdnimg.cn/20200718105306699.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl8zODUyNjE4MA==,size_16,color_FFFFFF,t_70)

下图是2M分页的线性地址转物理地址的过程示意图。
![2M分页的地址转换](https://img-blog.csdnimg.cn/20200718105329769.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl8zODUyNjE4MA==,size_16,color_FFFFFF,t_70)

下图是PDPTE、PDE-2M、PDE-4K、PTE各表项结构的示意图：
![PAE分页模式下各表项的结构](https://img-blog.csdnimg.cn/20200718114431402.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl8zODUyNjE4MA==,size_16,color_FFFFFF,t_70)

手册中给出了PDE-2M、PDE、PTE结构的详细解释，是下面三个表：
<br>

首先是PDE-2M表项结构解析：Table 4-9. Format of a PAE Page-Directory Entry that Maps a 2-MByte Page
| 位| 用途 |
|--|--|
| 0（P） | Present，必须等于1，表示该项指向一个2M分页 |
|1（R/W）|Read/Write，如果为0，不允许向2M的分页写数据|
|2（U/S）|User/supervisor，权限标志位，等于0则3环程序不能访问2M的分页|
|3（PWT）|Page-level write-through，用来间接确定访问2M分页所需要的内存属性|
|4（PCD）|Page-level cache disable，用来间接确定访问2M分页所需要的内存属性|
|5（A）|Accessed，标志这个2M分页是否已经被访问过|
|6（D）|Dirty，标志这个2M分页是否已经被写入过数据|
|7（PS）|Page size，分页大小标志位，必须等于1（否则这项的意义变为指向4K分页的页表）|
|8（G）|Gloabe，如果CR4.PGE等于1，该位用来确定这个页是否是全局共享的页|
|11:9|Ignored|
|12（PAT）|如果支持PAT，则用来间接确定访问2M分页所需要的内存属性|
|20:13|Reserved，保留位，必须是0|
|(M-1):21|2M分页的物理地址，36根物理线是35:21（15位），有的CPU有52根线的，是51:21|
|62:M|Reserved，保留位，必须是0|
|63（XD）|如果 IA32_EFER.NXE标志位等于1，则该页中的数据不可执行（从该页中获取指令将被禁止）； IA32_EFER.NXE等于0，该位位保留位，必须填0|
<br>

PDE表项结构解析：Table 4-10. Format of a PAE Page-Directory Entry that References a Page Table
| 位| 用途 |
|--|--|
| 0（P） | Present，必须等于1，表示该项指向一个页表 |
|1（R/W）|Read/Write，如果为0，不允许向其包含的1024个页（共2M）写数据|
|2（U/S）|User/supervisor，权限标志位，等于0则3环程序不能访问其包含的1024个页（共2M）|
|3（PWT）|Page-level write-through，用来间接确定访问页表所需要的内存属性|
|4（PCD）|Page-level cache disable，用来间接确定访问页表所需要的内存属性|
|5（A）|Accessed，标志这个表项已经被访问了（被线性地址翻译器访问了）|
|6（D）|Ignored|
|7（PS）|Page size，分页大小标志位，必须等于0|
|11:8|Ignored|
|(M-1):12|页表的物理地址，36根物理线的CPU是25:12（24位），有的CPU有52根线的，是51:12（40位）|
|62:M|Reserved，保留位，必须是0|
|63（XD）|如果 IA32_EFER.NXE标志位等于1，则该项指向的页表中的所有页（1024个页）中的数据不可执行； IA32_EFER.NXE等于0，该位位保留位，必须填0|
<br>

PTE表项结构解析：Table 4-11. Format of a PAE Page-Table Entry that Maps a 4-KByte Page
| 位| 用途 |
|--|--|
| 0（P） | Present，必须等于1，表示该项指向一个4K分页 |
|1（R/W）|Read/Write，如果为0，不允许向4K的分页写数据|
|2（U/S）|User/supervisor，权限标志位，等于0则3环程序不能访问4K的分页|
|3（PWT）|Page-level write-through，用来间接确定访问4K分页所需要的内存属性|
|4（PCD）|Page-level cache disable，用来间接确定访问4K分页所需要的内存属性|
|5（A）|Accessed，标志这个4K分页是否已经被访问过|
|6（D）|Dirty，标志这个4K分页是否已经被写入过数据|
|7（PAT）|如果支持PAT，则用来间接确定访问4K分页所需要的内存属性|
|8（G）|Gloabe，如果CR4.PGE等于1，该位用来确定这个页是否是全局共享的页|
|11:9|Ignored|
|(M-1):12|4K分页的物理地址，36根物理线的CPU是25:12（24位），有的CPU有52根线的，是51:12（40位）|
|62:M|Reserved，保留位，必须是0|
|63（XD）|如果 IA32_EFER.NXE标志位等于1，则该页中的数据不可执行（从该页中获取指令将被禁止）； IA32_EFER.NXE等于0，该位位保留位，必须填0|

<br>

# 实验题：实现页表浏览工具


## 1. 环境设置
测试操作系统：winxp sp3

开发环境配置：
	VS2015 实现MFC对话框工程
	[WDK 7.1.0](https://docs.microsoft.com/en-us/windows-hardware/drivers/other-wdk-downloads)开发winxp驱动

如何启用PAE模式？
读者可以检索关键字“winxp pae”，会搜索到很多关于winxp系统启用PAE模式的教程。
<br>


## 2. MFC浏览工具的代码实现
工具的使用效果如下图：

第一栏，显示系统中的所有进程，通过调用*CreateToolhelp32Snapshot* 实现遍历进程。
第二栏，展示页目录指针表的表项（PDPTE），选中上级目录的某个进程后，MFC程序调用驱动程序接口，加载该进程的PDPTE，最后展示有效表项。
第三栏，展示页目录表的表项（PDE），选中上级目录的某个PDPTE表项后，与上述类似流程。
第四栏，展示页表项（PTE），流程与上述类似。
![GetPages工具](https://img-blog.csdnimg.cn/20200721103245154.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl8zODUyNjE4MA==,size_16,color_FFFFFF,t_70)


**关于加载进程页表速度慢的问题：**
对于PAE模式，进程的页表项数量总共有：4 PDPTE x 512 PDE x 512 PTE
在最初版本的工具设计中，使用3层循环来遍历页表项，一次性加载某个进程的所有页表。但是出现加载速度慢的问题。所以改进了加载方式，变成根据索引加载一个PDPTE表项下的512项PDE，或者加载一个PDE表项下的512个PTE，这样改进之后，加载速度明显改善。
<br>
代码：定义保存页表数据的结构体
```cpp
namespace PAEPaging 
{
	struct PDE_T
	{
		PDE val;
		PTE PTEs[512];
		BOOL LoadedFlag;
	};
	struct PDPTE_T
	{
		PDPTE val;
		PDE_T PDEs[512];
		BOOL LoadedFlag;
	};
	struct PDPTE_TT
	{
		ULONG cr3Val;
		PDPTE_T PDPTEs[4];
	};
}
```
代码：调用驱动接口，加载页表的实现

```cpp
BOOL COperateKernel::LoadPages(DWORD dwPID, __in DWORD dwPDPTEIdx, __in DWORD dwPDEIdx, PAEPaging::PDPTE_TT* tt)
{
	...
    if (DeviceIoControl(hFile, IOCTL_GET_PAGES_PAE, inBuff, sizeof(inBuff), outBuff, sizeof(outBuff),
        &dwBytesRead, NULL))
    {
        DWORD dwOutBuffOffset = 0;
        // 加载 PDPTE 表
        if (dwPDPTEIdx == -1 && dwPDEIdx == -1)
        {
            for (int i = 0; i < 4; i++)
            {
                PAEPaging::PDPTE *pPDPTE = (PAEPaging::PDPTE*)((DWORD)outBuff + dwOutBuffOffset);
                tt->PDPTEs[i].val = *pPDPTE;
                dwOutBuffOffset += sizeof(PAEPaging::PDPTE);
            }
        }
        // 加载 PDE 表
        else if (dwPDPTEIdx != -1 && dwPDEIdx == -1)
        {
            for (int i = 0; i < 512; i++)
            {
                PAEPaging::PDE *pPDE = (PAEPaging::PDE*)((DWORD)outBuff + dwOutBuffOffset);
                tt->PDPTEs[dwPDPTEIdx].PDEs[i].val.uint64 = pPDE->uint64;
                dwOutBuffOffset += sizeof(PAEPaging::PDE);
            }
        }
        // 加载 PTE 表
        else if (dwPDPTEIdx != -1 && dwPDEIdx != -1)
        {
            for (int i = 0; i < 512; i++)
            {
                PAEPaging::PTE *pPTE = (PAEPaging::PTE*)((DWORD)outBuff + dwOutBuffOffset);
                tt->PDPTEs[dwPDPTEIdx].PDEs[dwPDEIdx].PTEs[i] = *pPTE;
                dwOutBuffOffset += sizeof(PAEPaging::PTE);
            }
        }
        bRet = TRUE;
    }

	...
}

```
<br>

## 3. 驱动代码的实现

```cpp
NTSTATUS GetProcessPages(PVOID pInBuff, PVOID pOutBuff, ULONG nOutLength, ULONG* nBytes)
{
    ...
    // 加载 PDPTE 表
    if (nPDPTEIdx == -1 && nPDEIdx == -1)
    {
        for (int i = 0; i < 4; i++)
        {
            ULONG64 paPDPTE = paCR3 + 8 * i;
            ULONG64 PDPTE = GetQuadByPA(paPDPTE);
            *(ULONG64*)((ULONG)pOutBuff + nWriteOffset) = PDPTE;
            nWriteOffset += sizeof(ULONG64);
        }
    }
    // 加载 PDE 表
    else if (nPDPTEIdx != -1 && nPDEIdx == -1)
    {
        ULONG64 paPDPTE = paCR3 + 8 * nPDPTEIdx;
        ULONG64 PDPTE = GetQuadByPA(paPDPTE) & 0xFFFFFF000;
        for (int i = 0; i < 512; i++)
        {
            ULONG64 paPDE = PDPTE + 8 * i;
            ULONG64 PDE = GetQuadByPA(paPDE);
            *(ULONG64*)((ULONG)pOutBuff + nWriteOffset) = PDE;
            nWriteOffset += sizeof(ULONG64);
        }
    }
    // 加载 PTE 表
    else if (nPDPTEIdx != -1 && nPDEIdx != -1)
    {
        ULONG64 paPDPTE = paCR3 + 8 * nPDPTEIdx;
        ULONG64 PDPTE = GetQuadByPA(paPDPTE) & 0xFFFFFF000;
        ULONG64 paPDE = PDPTE + 8 * nPDEIdx;
        ULONG64 PDE = GetQuadByPA(paPDE) & 0xFFFFFF000;
        for (int i = 0; i < 512; i++)
        {
            ULONG64 paPTE = PDE + 8 * i;
            ULONG64 PTE = GetQuadByPA(paPTE);
            *(ULONG64*)((ULONG)pOutBuff + nWriteOffset) = PTE;
            nWriteOffset += sizeof(ULONG64);
        }
    }
    ...
}

```
