const std = @import("std");

const Token = @import("tokens.zig");

pub const TokensBuffer = struct {
    tokens: []const Token.CommentedToken,

    pub fn init(tokens: []const Token.CommentedToken) TokensBuffer {
        return TokensBuffer{ .tokens = tokens };
    }

    pub fn format(
        self: TokensBuffer,
        _: []const u8,
        _: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        const len = self.tokens.len;
        if (len == 0) return;

        try std.fmt.format(writer, "{}", .{self.tokens[0].token});
        for (self.tokens[1..]) |tok| {
            try std.fmt.format(writer, " {}", .{tok.token});
        }
    }
};
