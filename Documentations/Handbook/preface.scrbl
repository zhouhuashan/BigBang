#lang scribble/manual

@(require "handbook.rkt")

@title[#:style '(toc unnumbered)]{序}

序章无关公司系统具体的技术实现，如果读者对计算机文化不感兴趣可以跳过去。
不过我依然建议@bold{不要跳过}：

@itemlist[#:style 'ordered
 @item{公司的系统运行在微软最新的技术平台上，过去的编程经验不总是能直接套用。
           而其中的差异也不是简单的“换了一套 API”，你的思维方式也必须跟进。
           或者，简单说句利益相关的话，@bold{以前的 C++ 代码不能直接用在新系统里}。}
  
 @item{@bold{代码应当方便别人(和自己)维护}，而这个话题天生众口难调。
           目前既然这个系统是我开发的，那我就很有必要说说我的风格和习惯。
           如果你愿意加入我的开发组，我会很欢迎你的建议。}]

@local-table-of-contents[]

@include-section{preface/introducation.scrbl}
@include-section{preface/style.scrbl}
