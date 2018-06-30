#include <gta3sc/sourceman.hpp>

namespace gta3sc
{
/// The preprocessor is a character stream over a source file.
///
/// This stream is stripped of comments, leading whitespaces in lines 
/// and line terminators are normalized to LF.
class Preprocessor
{
public:
    struct Snapshot;

    explicit Preprocessor(const SourceFile& source) :
        source(source)
    {
        this->cursor = source.view_with_terminator().begin();
        this->start_of_line = true;
        this->end_of_stream = false;
        this->num_block_comments = 0;
    }


    /// Gets the next character in the stream.
    char next();

    /// Checks whether the stream reached the end of file.
    bool eof() const;

    /// Checks whether the stream is inside a block comment.
    bool inside_block_comment() const;

    /// Obtains a snapshot of the stream state.
    auto tell() const -> Snapshot;

    /// Restores the stream state from a snapshot.
    void seek(const Snapshot&);

private:
    bool is_whitespace(SourceLocation) const;
    bool is_newline(SourceLocation) const;

public:
    struct Snapshot
    {
        SourceLocation cursor;
        bool start_of_line;
        bool end_of_stream;
        uint16_t num_block_comments;
    };

private:
    SourceLocation cursor;
    bool start_of_line;
    bool end_of_stream;
    uint16_t num_block_comments;

    const SourceFile& source;

    // Keep the snapshot as small as possible.
    static_assert(sizeof(Snapshot) <= 2 * sizeof(void*));
};
}
