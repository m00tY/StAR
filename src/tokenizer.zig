const std = @import("std");
const log = @import("log");
const CommentedToken = @import("tokens").CommentedToken;
const Comment = @import("tokens").Comment;
const Token = @import("tokens").Token;
const Special = @import("tokens").Special;
const Lambda = @import("tokens").Lambda;

const EOF = Token.EOF;
const Newline = Token.Newline;
const Semicolon = Token.Semicolon;
const Comma = Token.Comma;
const LParen = Token.LParen;
const RParen = Token.RParen;
const LBrace = Token.LBrace;
const RBrace = Token.RBrace;
const LBracket = Token.LBracket;
const RBracket = Token.RBracket;
const Literal = Token.Literal;
const Symbol = Token.Symbol;
const Help = Token.Help;
const LAssign = Token.LAssign;
const LowerEqual = Token.LowerEqual;
const SuperAssign = Token.SuperAssign;
const LowerThan = Token.LowerThan;
const GreaterEqual = Token.GreaterEqual;
const GreaterThan = Token.GreaterThan;
const Or = Token.Or;
const Pipe = Token.Pipe;
const VectorizedOr = Token.VectorizedOr;
const And = Token.And;
const VectorizedAnd = Token.VectorizedAnd;
const Equal = Token.Equal;
const OldAssign = Token.OldAssign;
const Dollar = Token.Dollar;
const RAssign = Token.RAssign;
const Minus = Token.Minus;
const NotEqual = Token.NotEqual;
const UnaryNot = Token.UnaryNot;
const Tilde = Token.Tilde;
const Slot = Token.Slot;
const NsGetInt = Token.NsGetInt;
const NsGet = Token.NsGet;
const ColonAssign = Token.ColonAssign;
const Colon = Token.Colon;
const Power = Token.Power;
const Multiply = Token.Multiply;
const Divide = Token.Divide;
const Plus = Token.Plus;
const Modulo = Token.Modulo;
const Function = Token.Function;
const Continue = Token.Continue;
const Break = Token.Break;
const For = Token.For;
const If = Token.If;
const Else = Token.Else;
const In = Token.In;
const While = Token.While;
const Repeat = Token.Repeat;

const SYMBOL_ENDING = [_]u8{ ' ', '(', ')', '{', '}', '#', ';', '\n', '\t', '\r', '+', '-', '/', '\\', '%', '*', '^', '!', '&', '|', '<', '>', '=', ',', '[', ']', '$', '`', '"' };

const Tokenizer = struct {
    offset: usize,
    it: usize,
    current_char: u8,
    source: std.mem.iterator(u8),
    raw_source: []const u8,
    allocator: *std.mem.Allocator,

    pub fn init(input: []const u8, allocator: *std.mem.Allocator) Tokenizer {
        return Tokenizer{
            .offset = 0,
            .it = 0,
            .current_char = '\\',
            .source = std.mem.iterator(input),
            .raw_source = input,
            .allocator = allocator,
        };
    }

    pub fn tokenize(self: *Tokenizer) []CommentedToken {
        var tokens = std.ArrayList(CommentedToken).init(self.allocator);
        self.next();
        while (self.it < self.raw_source.len) {
            switch (self.current_char) {
                ' ' | '\t' => self.next(),
                '\r' => {
                    self.next();
                    self.push_token(Newline, &tokens);
                    self.next();
                },
                '\n' => {
                    self.push_token(Newline, &tokens);
                    self.next();
                },
                ';' => {
                    self.push_token(Semicolon, &tokens);
                    self.next();
                },
                ',' => {
                    self.push_token(Comma, &tokens);
                    self.next();
                },
                '(' => {
                    self.push_token(LParen, &tokens);
                    self.next();
                },
                ')' => {
                    self.push_token(RParen, &tokens);
                    self.next();
                },
                '{' => {
                    self.push_token(LBrace, &tokens);
                    self.next();
                },
                '}' => {
                    self.push_token(RBrace, &tokens);
                    self.next();
                },
                '[' => {
                    self.push_token(LBracket, &tokens);
                    self.next();
                },
                ']' => {
                    self.push_token(RBracket, &tokens);
                    self.next();
                },
                '\'' | '"' => self.string_literal(&tokens),
                '*' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char == '*') {
                        self.push_token(Power, &tokens);
                        self.next();
                    } else {
                        self.push_token(Multiply, &tokens);
                    }
                    self.next();
                },
                '/' => {
                    self.push_token(Divide, &tokens);
                    self.next();
                },
                '^' => {
                    self.push_token(Power, &tokens);
                    self.next();
                },
                '+' => {
                    self.push_token(Plus, &tokens);
                    self.next();
                },
                '?' => {
                    self.push_token(Help, &tokens);
                    self.next();
                },
                '<' => {
                    const next_char = self.lookahead() orelse '\\';
                    switch (next_char) {
                        '-' => {
                            self.push_token(LAssign, &tokens);
                            self.next();
                        },
                        '=' => {
                            self.push_token(LowerEqual, &tokens);
                            self.next();
                        },
                        '<' => {
                            self.push_token(SuperAssign, &tokens);
                            self.next();
                            self.next();
                        },
                        else => self.push_token(LowerThan, &tokens),
                    }
                    self.next();
                },
                '>' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char == '=') {
                        self.push_token(GreaterEqual, &tokens);
                        self.next();
                    } else {
                        self.push_token(GreaterThan, &tokens);
                    }
                    self.next();
                },
                '|' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char == '|') {
                        self.push_token(Or, &tokens);
                        self.next();
                    } else if (next_char == '>') {
                        self.push_token(Pipe, &tokens);
                        self.next();
                    } else {
                        self.push_token(VectorizedOr, &tokens);
                    }
                    self.next();
                },
                '&' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char == '&') {
                        self.push_token(And, &tokens);
                        self.next();
                    } else {
                        self.push_token(VectorizedAnd, &tokens);
                    }
                    self.next();
                },
                '=' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char == '=') {
                        self.push_token(Equal, &tokens);
                        self.next();
                    } else {
                        self.push_token(OldAssign, &tokens);
                    }
                    self.next();
                },
                '$' => {
                    self.push_token(Dollar, &tokens);
                    self.next();
                },
                '-' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char == '>') {
                        self.push_token(RAssign, &tokens);
                        self.next();
                    } else {
                        self.push_token(Minus, &tokens);
                    }
                    self.next();
                },
                '!' => {
                    self.next();
                    if (self.current_char == '=') {
                        self.push_token(NotEqual, &tokens);
                        self.next();
                    } else {
                        self.push_token(UnaryNot, &tokens);
                    }
                },
                '.' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char >= 'a' and next_char <= 'z' or next_char >= 'A' and next_char <= 'Z') {
                        self.identifier(&tokens);
                    } else if (next_char >= '0' and next_char <= '9') {
                        self.number_literal(&tokens);
                    } else {
                        log.debug("Found not alphabetic and non-numeric character after a dot. Treating it as an identifier.");
                        self.identifier(&tokens);
                    }
                },
                '`' | '_' => self.identifier(&tokens),
                '%' => {
                    const next_char = self.lookahead() orelse '\\';
                    if (next_char == '%') {
                        self.push_token(Modulo, &tokens);
                        self.next();
                        self.next();
                    } else {
                        const custom_binary_start = self.it;
                        self.next();
                        while (self.current_char != '%') {
                            self.next();
                        }
                        const custom_binary_end = self.it;
                        self.push_token(Special(self.raw_source[custom_binary_start..custom_binary_end]), &tokens);
                        self.next();
                    }
                },
                'a'...'z', 'A'...'Z' => self.identifier_or_reserved(&tokens),
                '0'...'9' => self.number_literal(&tokens),
                '\\' => {
                    self.push_token(Lambda, &tokens);
                    self.next();
                },
                '#' => self.comment(&tokens),
                '~' => {
                    self.push_token(Tilde, &tokens);
                    self.next();
                },
                '@' => {
                    self.push_token(Slot, &tokens);
                    self.next();
                },
                ':' => {
                    self.next();
                    self.next(); // consume the ':'
                    const next_char_colon = self.lookahead();
                    if (self.current_char == ':' and next_char_colon == ':') {
                        self.push_token(NsGetInt, &tokens);
                        self.next();
                        self.next();
                    } else if (self.current_char == ':') {
                        self.push_token(NsGet, &tokens);
                        self.next();
                    } else if (self.current_char == '=') {
                        self.push_token(ColonAssign, &tokens);
                        self.next();
                    } else {
                        self.push_token(Colon, &tokens);
                    }
                },
                else => {},
            }
        }
        tokens.append(CommentedToken.new(EOF, self.offset));
        log.trace("Tokenized: {tokens:?}");
        return tokens.toOwnedSlice();
    }

    fn push_token(self: *Tokenizer, token: Token, tokens: *[]CommentedToken) void {
        tokens.append(CommentedToken.new(token, self.offset));
    }

    fn string_literal(self: *Tokenizer, tokens: *[]CommentedToken) void {
        const delimiter = self.current_char;
        const start_offset = self.offset;
        const start_it = self.it;
        var in_escape = false;
        self.next();
        while (self.current_char != delimiter or in_escape) {
            self.next();
            if (in_escape) {
                in_escape = !in_escape;
            } else if (self.current_char == '\\') {
                in_escape = true;
            }
        }
        tokens.append(CommentedToken.new(Literal(self.raw_source[start_it..self.it]), start_offset));
    }

    fn number_literal(self: *Tokenizer, tokens: *[]CommentedToken) void {
        const start_it = self.it;
        const next_char = self.lookahead();
        if (self.current_char == '0' and (next_char == 'x' or next_char == 'X')) {
            self.next();
            self.next();
            self.parse_hexadecimal(); // Implement this function if it doesn't exist
        } else {
            self.parse_decimal(); // Implement this function if it doesn't exist
        }
        self.push_token(Literal(self.raw_source[start_it..self.it]), tokens);
    }

    fn identifier(self: *Tokenizer, tokens: *[]CommentedToken) void {
        const start_it = self.it;
        var in_backticks = false;
        while ((self.it < self.raw_source.len) and (in_backticks or
            (self.current_char >= 'a' and self.current_char <= 'z') or
            (self.current_char >= 'A' and self.current_char <= 'Z') or
            (self.current_char >= '0' and self.current_char <= '9') or
            self.current_char == '.' or
            self.current_char == '_' or
            self.current_char == '`'))
        {
            if (self.current_char == '`') {
                in_backticks = !in_backticks;
            }
            self.next();
        }

        const symbol = self.raw_source[start_it..self.it];
        if (std.mem.eql(u8, symbol, "TRUE") or std.mem.eql(u8, symbol, "T")) {
            self.push_token(Literal("TRUE"), tokens);
        } else if (std.mem.eql(u8, symbol, "FALSE") or std.mem.eql(u8, symbol, "F")) {
            self.push_token(Literal("FALSE"), tokens);
        } else {
            self.push_token(Symbol(symbol), tokens);
        }
    }

    fn identifier_or_reserved(self: *Tokenizer, tokens: *[]CommentedToken) void {
        const start_it = self.it;
        while ((self.it < self.raw_source.len) and !SYMBOL_ENDING.contains(self.current_char)) {
            self.next();
        }
        const symbol = self.raw_source[start_it..self.it];

        if (std.mem.eql(u8, symbol, "continue")) {
            self.push_token(Continue, tokens);
        } else if (std.mem.eql(u8, symbol, "break")) {
            self.push_token(Break, tokens);
        } else if (std.mem.eql(u8, symbol, "for")) {
            self.push_token(For, tokens);
        } else if (std.mem.eql(u8, symbol, "if")) {
            self.push_token(If, tokens);
        } else if (std.mem.eql(u8, symbol, "else")) {
            self.push_token(Else, tokens);
        } else if (std.mem.eql(u8, symbol, "in")) {
            self.push_token(In, tokens);
        } else if (std.mem.eql(u8, symbol, "while")) {
            self.push_token(While, tokens);
        } else if (std.mem.eql(u8, symbol, "repeat")) {
            self.push_token(Repeat, tokens);
        } else if (std.mem.eql(u8, symbol, "function")) {
            self.push_token(Function, tokens);
        } else if (std.mem.eql(u8, symbol, "TRUE") or std.mem.eql(u8, symbol, "T")) {
            self.push_token(Literal("TRUE"), tokens);
        } else if (std.mem.eql(u8, symbol, "FALSE") or std.mem.eql(u8, symbol, "F")) {
            self.push_token(Literal("FALSE"), tokens);
        } else {
            self.push_token(Symbol(symbol), tokens);
        }
    }
    fn lookahead(self: *Tokenizer) ?u8 {
        if (self.it + 1 >= self.raw_source.len) return null;
        return self.raw_source[self.it + 1];
    }

    fn next(self: *Tokenizer) void {
        if (self.it < self.raw_source.len) {
            self.current_char = self.raw_source[self.it];
            self.offset += 1;
            self.it += 1;
        } else {
            self.current_char = 0;
        }
    }

    fn comment(self: *Tokenizer, tokens: *[]CommentedToken) void {
        const start_it = self.it;
        self.next();
        while (self.current_char != '\n' and self.current_char != 0) {
            self.next();
        }
        tokens.append(CommentedToken.comment(Comment(self.raw_source[start_it..self.it])));
    }
};
