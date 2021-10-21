#### Comments

1. 处理嵌套的comments，设置全局变量comment_level和lex的状态，当读取到第一个/\*时进入comment状态同时增加comment_level的值，在comment状态下每次读取到/\*都会增加comment_level的值，而读取到\*/则会减少comment_level的值，当减少至0时退出comment状态。
2. 通过状态的转换，在comment状态内忽略读取到的字符，在其他状态下遇到\*/为出错，这种规则实现comment的嵌套

#### String

1. 初始状态下遇到"进入string状态
2. 在string状态下遇到"返回初始状态
3. 当string状态下遇到符合规则的转义字符，解析并产生相关的动作

#### Error Handling

1. 遇到不符合语义的字符，进行adjust()操作，后移位置，简单地忽略当前字符，不影响后续的操作

#### EOF Handling

1. EOF时结束