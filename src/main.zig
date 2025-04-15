const std = @import("std");
pub const parse = @import("parse.zig");

pub fn main() !void {
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    const gpa = general_purpose_allocator.allocator();

    const args = try std.process.argsAlloc(gpa);
    defer std.process.argsFree(gpa, args);

    if (args.len < 3) {
        std.debug.print("Usage: star <flag> <filename>\n", .{});
        return;
    }

    if (std.mem.eql(u8, args[1], "-p")) {
        try runParse(gpa, args[2]);
    } else {
        std.debug.print("Unknown flag: {s}\n", .{args[1]});
    }
}

pub fn runParse(allocator: std.mem.Allocator, filename: []const u8) !void {
    const blocks = try parse.generateSignatureBlocks(allocator, filename);
    // Assuming blocks is a slice of something printable
    for (blocks) |block| {
        std.debug.print("{any}\n", .{block});
    }
}
