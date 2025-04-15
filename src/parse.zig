const std = @import("std");
const regex = std.regex;
const Allocator = std.mem.Allocator;

const Tag = enum { param, return_ };

const TypeSignature: [6][]const u8 = .{ "logical", "integer", "double", "complex", "character", "raw" };

const ParamTypeMapping = struct {
    name: []const u8,
    tag: Tag,
    type_signature: TypeSignature,
};

const SignatureBlock = struct {
    filename: []const u8,
    start_line: usize,
    end_line: usize,
    tags: []Tag,
    parameters: []const []const u8,
    types: []TypeSignature,
    mapping: []ParamTypeMapping,
    fun_name: ?[]const u8,
    occurence: usize,
};

const validLinterTags = &.{ Tag.param, Tag.return_ };

// Read lines from a file
fn readLines(file_name: []const u8) ![]const []const u8 {
    const allocator = std.heap.page_allocator;
    const file = try std.fs.cwd().openFile(file_name, .{ .read = true });
    defer file.close();

    const file_contents = try file.readToEndAlloc(allocator);
    const lines = std.mem.split(u8, file_contents, "\n");

    var line_strings: []const []const u8 = &{};
    for (lines) |line| {
        line_strings = &line_strings ++ &.{line};
    }

    return line_strings;
}

// Regex matching function
fn strMatch(input: []const u8, pattern: []const u8) ![]const u8 {
    const re = try regex.Regex.compile(pattern);
    const result = try re.match(input);

    var match_groups: []const u8 = &{};
    for (result.groups) |group| {
        match_groups = &match_groups ++ &.{group};
    }

    return match_groups;
}

fn strReplace(input: []const u8, old: []const u8, new: []const u8) []const u8 {
    var result: []u8 = &{};
    var i = 0;

    while (i < input.len) {
        if (i + old.len <= input.len and input[i .. i + old.len] == old) {
            result = &result ++ new;
            i += old.len;
        } else {
            result = &result ++ &.{input[i]};
            i += 1;
        }
    }

    return result;
}

fn createSignatureBlock(file_name: []const u8, start_line: usize, end_line: usize) SignatureBlock {
    return SignatureBlock{
        .filename = file_name,
        .startline = start_line,
        .endline = end_line,
        .tags = &{},
        .parameters = &{},
        .types = &{},
        .mapping = &{},
        .fun_name = null,
    };
}

fn validateLinterTag(block: SignatureBlock) []Tag {
    var validTags: []Tag = &{};
    const filename = block.filename;
    const startline = block.startline;
    const endline = block.endline;
    const lines = readLines(filename);
    const blocklines = lines[startline..endline];

    for (blocklines) |line| {
        const match = strMatch(line, "#\\s*(@param|@return)");
        if (match[1] != null) {
            if (match[1] == validLinterTags[0] or match[2] == validLinterTags[1]) {
                validTags.append(match[1]);
            }
        }
    }
    return validTags;
}

fn validateParameterSymbols(block: SignatureBlock) []u8 {
    var parameterSymbols: []u8 = &{};
    const filename = block.filename;
    const startline = block.startline;
    const endline = block.endline;
    const lines = readLines(filename);
    const blocklines = lines[startline..endline];

    for (blocklines) |line| {
        const match = strMatch(line, "#\\s*(@param)\\s+(\\S+):");
        if (match[3] != null) {
            parameterSymbols.append(match[2]);
        }
    }
    return (parameterSymbols);
}

fn validateTypeSignature(block: SignatureBlock) []u8 {
    var validTypes: []u8 = &{};
    const filename = block.filename;
    const startline = block.startline;
    const endline = block.endline;
    const lines = readLines(filename);
    const blocklines = lines[startline..endline];

    for (blocklines) |line| {
        const match = strMatch(line, "#\\s*(@param|@return)\\s+(\\S+):?\\s*(\\S+)?");
        if (match[3] != null and (match[3] == TypeSignature[0] or
            match[3] == TypeSignature[1] or
            match[3] == TypeSignature[2] or
            match[3] == TypeSignature[3] or
            match[3] == TypeSignature[4] or
            match[3] == TypeSignature[5]))
        {
            validTypes.append(match[3]);
        } else if (match[2] != null and (match[3] == TypeSignature[0] or
            match[3] == TypeSignature[1] or
            match[3] == TypeSignature[2] or
            match[3] == TypeSignature[3] or
            match[3] == TypeSignature[4] or
            match[3] == TypeSignature[5]))
        {
            validTypes.append(match[2]);
        }
    }
    return (validTypes);
}

fn identifyConsecutiveLineNumbers(numbers: []const i32) []const []i32 {
    if (numbers.len == 0) {
        return &[]const []i32{};
    }

    var groups: []const []i32 = &[]const []i32{};
    var currentGroup: []i32 = &[_]i32{numbers[0]};

    var i = numbers[0];
    for (numbers) |number| {
        if (number == numbers[i - 1] + 1) {
            currentGroup = currentGroup ++ &[_]i32{number};
        } else {
            groups = groups ++ &[]const []i32{currentGroup};
            currentGroup = &[_]i32{number};
        }
        i += 1;
    }

    groups = groups ++ &[]const []i32{currentGroup}; // append the last group
    return groups;
}

pub fn generateSignatureBlocks(filename: []u8) []SignatureBlock {
    var signatureBlocks: []SignatureBlock = &{};
    var regexMatches: []usize = &{};
    const lines = readLines(filename);

    for (lines) |i| {
        if (strMatch(lines[i], "#\\s*@\\S+")) {
            regexMatches.append(i);
        }
    }

    const consecutiveLineNumbers = identifyConsecutiveLineNumbers(regexMatches);

    for (consecutiveLineNumbers) |lineGroup| {
        const start: u32 = @intCast(lineGroup[1]);
        const end: u32 = @intCast(lineGroup.len());
        const block = createSignatureBlock(filename, start, end);
        const validTags = validateLinterTag(block);
        const parameterSymbols = validateParameterSymbols(block);
        const validTypes = validateTypeSignature(block);

        const functionName = null;

        for (lines[(end + 1)..]) |line| {
            if (std.mem.indexOf(u8, line, "function") != null and
                std.mem.indexOf(u8, line, "<-") != null)
            {
                const arrow_index = std.mem.indexOf(u8, line, "<-").?;
                const name_part = line[0..arrow_index];
                const trimmed = std.mem.trim(u8, name_part, " \t");
                functionName = trimmed;
                break;
            }
        }

        var paramTypeMap = &{};
        const blocklines = lines[start..end];

        for (blocklines) |line| {
            const match = strMatch(line, "#\\s*(@param|@return)\\s+(\\S+):?\\s*(\\S+)?");
            if (match[1] != null) {
                const tag = match[1];
                const name = if (std.mem.endsWith(u8, match[3], ":"))
                    match[2][0 .. match[2].len - 1]
                else
                    match[2];

                const typeval = match[3];

                if (typeval != null and (typeval == TypeSignature[0] or
                    typeval == TypeSignature[1] or
                    typeval == TypeSignature[2] or
                    typeval == TypeSignature[3] or
                    typeval == TypeSignature[4] or
                    typeval == TypeSignature[5]))
                {
                    paramTypeMap.put(name, &[_][]const u8{ tag, typeval }) catch unreachable;
                } else if (std.mem.indexOfScalar([]const []const u8, TypeSignature, name) != null) {
                    paramTypeMap.put(name, &[_][]const u8{ tag, name }) catch unreachable;
                }
            }
        }
        try signatureBlocks.append(SignatureBlock{
            .block = block,
            .tags = validTags,
            .parameters = parameterSymbols,
            .types = validTypes,
            .mapping = paramTypeMap,
            .funName = functionName,
        });
    }
    return signatureBlocks;
}
