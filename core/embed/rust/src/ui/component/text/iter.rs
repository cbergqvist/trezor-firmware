use crate::ui::{component::LineBreaking, display::Font};
use core::iter;

#[derive(Copy, Clone, Eq, PartialEq, Debug)]
struct LineBreak {
    /// Index of character **after** the line-break.
    offset: usize,
    /// Distance from the last line-break of the sequence, in pixels.
    width: i32,
    style: BreakStyle,
}

#[derive(Copy, Clone, Eq, PartialEq, Debug)]
enum BreakStyle {
    Hard,
    AtWhitespaceOrWordBoundary,
    InsideWord,
}

fn limit_line_breaks(
    breaks: impl Iterator<Item = LineBreak>,
    line_height: i32,
    available_height: i32,
) -> impl Iterator<Item = LineBreak> {
    breaks.take(available_height as usize / line_height as usize)
}

#[derive(Copy, Clone, Eq, PartialEq, Debug)]
enum Appendix {
    None,
    Hyphen,
}

#[derive(Copy, Clone, Eq, PartialEq, Debug)]
struct Span<'a> {
    text: &'a str,
    append: Appendix,
}

fn break_text_to_spans(
    text: &str,
    text_font: impl GlyphMetrics,
    hyphen_font: impl GlyphMetrics,
    breaking: LineBreaking,
    available_width: i32,
) -> impl Iterator<Item = Span> {
    let mut finished = false;
    let mut last_break = LineBreak {
        offset: 0,
        width: 0,
        style: BreakStyle::AtWhitespaceOrWordBoundary,
    };
    let mut breaks = select_line_breaks(
        text.char_indices(),
        text_font,
        hyphen_font,
        breaking,
        available_width,
    );
    iter::from_fn(move || {
        if finished {
            None
        } else if let Some(lb) = breaks.next() {
            let start_of_line = last_break.offset;
            let end_of_line = lb.offset; // Not inclusive.
            last_break = lb;
            Some(Span {
                text: &text[start_of_line..end_of_line],
                append: match lb.style {
                    BreakStyle::Hard | BreakStyle::AtWhitespaceOrWordBoundary => Appendix::None,
                    BreakStyle::InsideWord => Appendix::Hyphen,
                },
            })
        } else {
            finished = true;
            Some(Span {
                text: &text[last_break.offset..],
                append: Appendix::None,
            })
        }
    })
}

fn select_line_breaks(
    chars: impl Iterator<Item = (usize, char)>,
    text_font: impl GlyphMetrics,
    hyphen_font: impl GlyphMetrics,
    breaking: LineBreaking,
    available_width: i32,
) -> impl Iterator<Item = LineBreak> {
    let hyphen_width = hyphen_font.char_width('-');
    let ellipsis_width = 3 * hyphen_font.char_width('.');
    let line_height = text_font.line_height();

    let mut proposed = None;
    let mut line_width = 0;
    let mut total_height = line_height;
    let mut found_any_whitespace = false;

    chars.filter_map(move |(offset, ch)| {
        let char_width = text_font.char_width(ch);
        let exceeds_available_width = line_width + char_width > available_width;
        let have_space_for_break = line_width + char_width + hyphen_width <= available_width;
        let can_break_word =
            matches!(breaking, LineBreaking::BreakWordsAndInsertHyphen) || !found_any_whitespace;

        let next_offset = offset + ch.len_utf8();
        let next_line_width = line_width + char_width;

        let break_line = match ch {
            '\n' | '\r' => {
                // Immediate hard break.
                Some(LineBreak {
                    offset: next_offset,
                    width: next_line_width,
                    style: BreakStyle::Hard,
                })
            }
            ' ' | '\t' => {
                // Whitespace, propose a line-break after this character.
                proposed = Some(LineBreak {
                    offset: next_offset,
                    width: next_line_width,
                    style: BreakStyle::AtWhitespaceOrWordBoundary,
                });
                found_any_whitespace = true;
                None
            }
            _ if have_space_for_break && can_break_word => {
                // Propose a word-break after this character. In case the next character is
                // whitespace, the proposed word break is replaced by a whitespace break.
                proposed = Some(LineBreak {
                    offset: next_offset,
                    width: next_line_width,
                    style: BreakStyle::InsideWord,
                });
                None
            }
            _ if exceeds_available_width => {
                // Consume the last proposed line-break. In case we don't have anything
                // proposed, we hard-break immediately before this character. This only happens
                // if the first character of the line doesn't fit.
                Some(proposed.unwrap_or(LineBreak {
                    offset,
                    width: line_width,
                    style: BreakStyle::Hard,
                }))
            }
            _ => None,
        };
        if let Some(br) = break_line {
            // Reset the state.
            proposed = None;
            line_width -= br.width;
            total_height += line_height;
            found_any_whitespace = false;
        }
        // Shift cursor.
        line_width += char_width;
        break_line
    })
}

pub fn count_lines(
    chars: impl Iterator<Item = (usize, char)>,
    text_font: impl GlyphMetrics,
    hyphen_font: impl GlyphMetrics,
    breaking: LineBreaking,
    available_width: i32,
) -> usize {
    select_line_breaks(chars, text_font, hyphen_font, breaking, available_width).count() + 1
}

pub fn select_pages(
    chars: impl Iterator<Item = (usize, char)>,
    text_font: impl GlyphMetrics,
    hyphen_font: impl GlyphMetrics,
    breaking: LineBreaking,
    available_width: i32,
    available_height: i32,
) -> impl Iterator<Item = usize> {
    let line_height = text_font.line_height();
    let mut consumed_height = 0;
    let mut breaks = select_line_breaks(chars, text_font, hyphen_font, breaking, available_width);
    let mut finished = false;
    let mut previous_page_at = 0;
    iter::from_fn(move || {
        while let Some(br) = breaks.next() {
            consumed_height += line_height;
            if consumed_height > available_height {
                consumed_height = 0;
                let result = Some(previous_page_at);
                previous_page_at = br.offset;
                return result;
            }
        }
        if !finished {
            finished = true;
            Some(previous_page_at)
        } else {
            None
        }
    })
}

pub trait GlyphMetrics {
    fn char_width(&self, ch: char) -> i32;
    fn line_height(&self) -> i32;
}

impl GlyphMetrics for Font {
    fn char_width(&self, ch: char) -> i32 {
        Font::char_width(*self, ch)
    }

    fn line_height(&self) -> i32 {
        Font::line_height(*self)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    pub struct Fixed {
        pub width: i32,
        pub height: i32,
    }

    impl GlyphMetrics for Fixed {
        fn char_width(&self, _ch: char) -> i32 {
            if _ch == '\n' {
                0
            } else {
                self.width
            }
        }

        fn line_height(&self) -> i32 {
            self.height
        }
    }

    const FIXED_FONT: Fixed = Fixed {
        width: 1,
        height: 1,
    };

    #[test]
    fn test_span() {
        assert_eq!(spans_from("hello", 5), vec![("hello", false)]);
        assert_eq!(spans_from("", 5), vec![("", false)]);
        assert_eq!(
            spans_from("hello world", 5),
            vec![("hello ", false), ("world", false)]
        );
        assert_eq!(
            spans_from("hello\nworld", 5),
            vec![("hello\n", false), ("world", false)]
        );
    }

    #[test]
    fn test_leading_trailing() {
        assert_eq!(
            spans_from("\nhello\nworld\n", 5),
            vec![
                ("\n", false),
                ("hello\n", false),
                ("world\n", false),
                ("", false)
            ]
        );
    }

    #[test]
    fn test_long_word() {
        assert_eq!(
            spans_from("Constantinople", 7),
            vec![("Consta", true), ("ntinop", true), ("le", false)]
        );

        assert_eq!(
            spans_from("Down with the establishment!", 5),
            vec![
                ("Down ", false),
                ("with ", false),
                ("the ", false),
                ("esta", true),
                ("blis", true),
                ("hmen", true),
                ("t!", false),
            ]
        );
    }

    #[test]
    fn test_char_boundary() {
        assert_eq!(
            spans_from("+ěščřžýáíé", 5),
            vec![("+ěšč", true), ("řžýá", true), ("íé", false)]
        );
    }

    fn spans_from(text: &str, max_width: i32) -> Vec<(&str, bool)> {
        break_text_to_spans(
            text,
            FIXED_FONT,
            FIXED_FONT,
            LineBreaking::BreakAtWhitespace,
            max_width,
        )
        .map(|span| (span.text, matches!(span.append, Appendix::Hyphen)))
        .collect()
    }
}
