%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */

digit     [0-9]
letter    [A-Za-z]
underline \_
id        {letter}({letter}|{digit}|{underline})*
integer   {digit}+
ignore    \\([ \f\n\r\t\v]+)\\
esc       (\\n|\\t|\\\\|\\\"|(\\\^{letter})|(\\{digit}{3}))

 /* TODO: Put your lab2 code here */
 /* exclusive condition declaration */
%x COMMENT STR IGNORE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA     ,
  *   Parser::COLON     :
  *   Parser::SEMICOLON ;
  *   Parser::LPAREN    (
  *   Parser::RPAREN    )
  *   Parser::LBRACK    [
  *   Parser::RBRACK    ]
  *   Parser::LBRACE    {
  *   Parser::RBRACE    }
  *   Parser::DOT       .
  *   Parser::PLUS      +
  *   Parser::MINUS     -
  *   Parser::TIMES     *
  *   Parser::DIVIDE    /
  *   Parser::EQ        =
  *   Parser::NEQ       <>
  *   Parser::LT        <
  *   Parser::LE        <=
  *   Parser::GT        >
  *   Parser::GE        >=
  *   Parser::AND       &
  *   Parser::OR        |
  *   Parser::ASSIGN    :=
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /* reserved words */
<INITIAL>"array"     {adjust(); return Parser::ARRAY;}
<INITIAL>"if"        {adjust(); return Parser::IF;}
<INITIAL>"then"      {adjust(); return Parser::THEN;}
<INITIAL>"else"      {adjust(); return Parser::ELSE;}
<INITIAL>"while"     {adjust(); return Parser::WHILE;}
<INITIAL>"for"       {adjust(); return Parser::FOR;}
<INITIAL>"to"        {adjust(); return Parser::TO;}
<INITIAL>"do"        {adjust(); return Parser::DO;}
<INITIAL>"let"       {adjust(); return Parser::LET;}
<INITIAL>"in"        {adjust(); return Parser::IN;}
<INITIAL>"end"       {adjust(); return Parser::END;}
<INITIAL>"of"        {adjust(); return Parser::OF;}
<INITIAL>"break"     {adjust(); return Parser::BREAK;}
<INITIAL>"nil"       {adjust(); return Parser::NIL;}
<INITIAL>"function"  {adjust(); return Parser::FUNCTION;}
<INITIAL>"var"       {adjust(); return Parser::VAR;}
<INITIAL>"type"      {adjust(); return Parser::TYPE;}
 /* TODO: Put your lab2 code here */
<INITIAL>\,      {adjust(); return Parser::COMMA;}
<INITIAL>\:      {adjust(); return Parser::COLON;}
<INITIAL>\;      {adjust(); return Parser::SEMICOLON;}
<INITIAL>\(      {adjust(); return Parser::LPAREN;}
<INITIAL>\)      {adjust(); return Parser::RPAREN;}
<INITIAL>\[      {adjust(); return Parser::LBRACK;}
<INITIAL>\]      {adjust(); return Parser::RBRACK;}
<INITIAL>\{      {adjust(); return Parser::LBRACE;}
<INITIAL>\}      {adjust(); return Parser::RBRACE;}
<INITIAL>\.      {adjust(); return Parser::DOT;}
<INITIAL>\+      {adjust(); return Parser::PLUS;}
<INITIAL>\-      {adjust(); return Parser::MINUS;}
<INITIAL>\*      {adjust(); return Parser::TIMES;}
<INITIAL>\/      {adjust(); return Parser::DIVIDE;}
<INITIAL>\=      {adjust(); return Parser::EQ;}
<INITIAL>\<\>    {adjust(); return Parser::NEQ;}
<INITIAL>\<\=    {adjust(); return Parser::LE;}
<INITIAL>\>\=    {adjust(); return Parser::GE;}
<INITIAL>\<      {adjust(); return Parser::LT;}
<INITIAL>\>      {adjust(); return Parser::GT;}
<INITIAL>\&      {adjust(); return Parser::AND;}
<INITIAL>\|      {adjust(); return Parser::OR;}
<INITIAL>\:\=    {adjust(); return Parser::ASSIGN;}
<INITIAL>"/*"    {adjust(); comment_level_=1; begin(StartCondition__::COMMENT);}
<COMMENT>"/*"    {adjust(); comment_level_++; }
<COMMENT>"*/"    {adjust(); comment_level_--; if(comment_level_<0) errormsg_->Error(errormsg_->tok_pos_, "nested comment error");if(comment_level_==0) begin(StartCondition__::INITIAL); }
<COMMENT>"\n"    {adjust(); errormsg_->Newline();}
<COMMENT>.       {adjust();}
<STR>{ignore}    {adjustStr();}
<STR>\"          {adjustStr(); begin(StartCondition__::INITIAL); setMatched(string_buf_); return Parser::STRING;}
<STR>\\n         {adjustStr(); string_buf_+='\n';}
<STR>\\t         {adjustStr(); string_buf_+='\t';}
<STR>\\\\        {adjustStr(); string_buf_+='\\';}
<STR>\\\"        {adjustStr(); string_buf_+='\"';}
<STR>\\[0-9]{3}  {adjustStr(); string_tmp_=matched(); string_tmp_=string_tmp_.substr(1,3); string_buf_+=((char)(atoi(string_tmp_.c_str())));}
<STR>\\\^[A-Z]   {adjustStr(); string_tmp_=matched(); string_buf_+=(string_tmp_[2]-64);}
<STR>\\          {adjustStr(); errormsg_->Error(errormsg_->tok_pos_, "illegal escape character");}
<STR>.           {adjustStr(); string_buf_+=matched();}
<INITIAL>\"      {adjust(); begin(StartCondition__::STR); string_buf_="";}
<INITIAL>{integer}   {adjust(); return Parser::INT;}
<INITIAL>{id}    {adjust(); return Parser::ID;}

 /*
  * skip white space chars.
  * space, tabs and LF
  */
<INITIAL>[ \t]+  {adjust();}
<INITIAL>"\n"    {adjust(); errormsg_->Newline();}
 /* illegal input */
<*>.             {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
