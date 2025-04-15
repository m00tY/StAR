const std = @import("std");

pub const CommentedToken = struct {
    token: []const u8,
    offset: usize,
    leadingComments: ?[]const []const u8,
    inlineComment: ?[]const []const u8,

    pub fn init(token: Token, offset: usize) CommentedToken {
        return CommentedToken{
            .token = token,
            .offset = offset,
            .leadingComments = null,
            .inlineComment = null,
        };
    }

    pub fn withComments(
        token: Token,
        offset: usize,
        leadingComments: ?[]const []const u8,
        inlineComment: ?[]const []const u8,
    ) CommentedToken {
        return CommentedToken{
            .token = token,
            .offset = offset,
            .leading_comments = leadingComments,
            .inline_comment = inlineComment,
        };
    }

    pub fn eql(self: CommentedToken, other: CommentedToken) bool {
        return std.meta.eql(self.token, other.token);
    }

    pub fn asTokenPtr(self: *CommentedToken) *Token {
        return &self.token;
    }

    pub fn format(
        self: CommentedToken,
        _: []const u8, //format
        _: std.fmt.FormatOptions, //options
        writer: anytype,
    ) !void {
        try std.fmt.format(writer, "Token: {}, Offset: {}, Leading Comments: {any}, Inline: {any}", .{ self.token, self.offset, self.leadingComments, self.inlineComment });
    }
};

pub const Token = union(enum) {
    Symbol: []const u8,
    Literal: []const u8,
    Semicolon,
    Newline,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,

    Continue,
    Break,
    Stop,

    If,
    Else,
    While,
    For,
    Repeat,
    In,
    Function,
    Lambda,

    LAssign,
    SuperAssign,
    ColonAssign,
    RAssign,
    OldAssign,
    Equal,
    NotEqual,
    LowerThan,
    GreaterThan,
    LowerEqual,
    GreaterEqual,
    Power,
    Divide,
    Multiply,
    Minus,
    Plus,
    Help,
    And,
    VectorizedAnd,
    Or,
    VectorizedOr,
    Dollar,
    Pipe,
    Modulo,
    NsGet,
    NsGetInt,
    Tilde,
    Colon,
    Slot,
    Special: []const u8,

    UnaryNot,

    InlineComment: []const u8,
    Comment: []const u8,

    EOF,
};

pub fn commentedTokens(comptime tokens: anytype) []const CommentedToken {
    const count = tokens.len;
    var result: [count]CommentedToken = undefined;

    inline for (tokens, 0..) |tok, i| {
        result[i] = CommentedToken.init(tok, 0);
    }

    return &result;
}
