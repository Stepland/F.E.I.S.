#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "better_metadata.hpp"
#include "better_notes.hpp"
#include "better_song.hpp"

class EditorState;


class HistoryItem {
public:
    explicit HistoryItem(std::string message = "") :
        message(std::move(message)) {};

    const std::string& get_message() const;
    virtual void do_action(EditorState& ed) const {};
    virtual void undo_action(EditorState& ed) const {};

    virtual ~HistoryItem() = default;

protected:
    std::string message;
};


class AddNotes : public HistoryItem {
public:
    AddNotes(
        const std::string& difficulty_name,
        const better::Notes& notes
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;

protected:
    std::string difficulty_name;
    better::Notes notes;
};


class RemoveNotes : public AddNotes {
public:
    RemoveNotes(
        const std::string& difficulty_name,
        const better::Notes& notes
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;
};


class RerateChart : public HistoryItem {
public:
    RerateChart(
        const std::string& chart,
        const std::optional<Decimal>& old_level,
        const std::optional<Decimal>& new_level
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;

protected:
    std::string chart;
    std::optional<Decimal> old_level;
    std::optional<Decimal> new_level;
};


class RenameChart : public HistoryItem {
public:
    RenameChart(
        const std::string& old_name,
        const std::string& new_name
    );

    void do_action(EditorState& ed) const override;
    void undo_action(EditorState& ed) const override;

protected:
    std::string old_name;
    std::string new_name;
};

template<class T>
class ChangeMetadataValue : public HistoryItem {
public:

    ChangeMetadataValue(const T& old_value, const T& new_value) :
        old_value(old_value),
        new_value(new_value)
    {

    }

    void do_action(EditorState& ed) const override {
        set_value(ed, new_value);
    }

    void undo_action(EditorState& ed) const override {
        set_value(ed, old_value);
    }
protected:
    virtual void set_value(EditorState& ed, const T& value) const = 0;

    T old_value;
    T new_value;
};

class ChangeTitle : public ChangeMetadataValue<std::string> {
public:
    ChangeTitle(const std::string& old_value, const std::string& new_value);
protected:
    void set_value(EditorState& ed, const std::string& value) const override;
};

class ChangeArtist : public ChangeMetadataValue<std::string> {
public:
    ChangeArtist(const std::string& old_value, const std::string& new_value);
protected:
    void set_value(EditorState& ed, const std::string& value) const override;
};

class ChangeAudio : public ChangeMetadataValue<std::string> {
public:
    ChangeAudio(const std::string& old_value, const std::string& new_value);
protected:
    void set_value(EditorState& ed, const std::string& value) const override;
};

class ChangeJacket : public ChangeMetadataValue<std::string> {
public:
    ChangeJacket(const std::string& old_value, const std::string& new_value);;
protected:
    void set_value(EditorState& ed, const std::string& value) const override;
};

using PreviewState = std::variant<better::PreviewLoop, std::string>;

template <>
struct fmt::formatter<PreviewState>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const PreviewState& c, FormatContext& ctx) {
        const auto format_ = VariantVisitor {
            [&](const better::PreviewLoop& loop) {
                return format_to(ctx.out(), "{}", loop);
            },
            [&](const std::string& file) {
                return format_to(ctx.out(), "\"{}\"", file);
            },
        };
        return std::visit(format_, c);
    }
};

class ChangePreview : public ChangeMetadataValue<PreviewState> {
public:
    ChangePreview(const PreviewState& old_value, const PreviewState& new_value);
protected:
    void set_value(EditorState& ed, const PreviewState& value) const override;
};
