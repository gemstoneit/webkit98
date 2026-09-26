/*
 * Copyright (C) 2014 Igalia S.L.
 * Copyright (C) 2026 Gemstone IT Services Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "Win98MiniEditorClient.h"

#include "Document.h"
#include "Editor.h"
#include "EditorClient.h"
#include "EventNames.h"
#include "EventTargetInlines.h"
#include "FrameDestructionObserverInlines.h"
#include "HTMLInputElement.h"
#include "KeyboardEvent.h"
#include "LocalFrame.h"
#include "LocalFrameInlines.h"
#include "Node.h"
#include "NodeDocument.h"
#include "PlatformKeyboardEvent.h"
#include "TextCheckerClient.h"
#include "WindowsKeyboardCodes.h"
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

static constexpr unsigned ctrlKey = 1 << 0;
static constexpr unsigned altKey = 1 << 1;
static constexpr unsigned shiftKey = 1 << 2;
static constexpr unsigned metaKey = 1 << 3;

struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static constexpr KeyDownEntry keyDownEntries[] = {
    { VK_LEFT, 0, "MoveLeft" },
    { VK_LEFT, shiftKey, "MoveLeftAndModifySelection" },
    { VK_LEFT, ctrlKey, "MoveWordLeft" },
    { VK_LEFT, ctrlKey | shiftKey, "MoveWordLeftAndModifySelection" },
    { VK_RIGHT, 0, "MoveRight" },
    { VK_RIGHT, shiftKey, "MoveRightAndModifySelection" },
    { VK_RIGHT, ctrlKey, "MoveWordRight" },
    { VK_RIGHT, ctrlKey | shiftKey, "MoveWordRightAndModifySelection" },
    { VK_UP, 0, "MoveUp" },
    { VK_UP, shiftKey, "MoveUpAndModifySelection" },
    { VK_UP, ctrlKey | shiftKey, "MoveParagraphBackwardAndModifySelection" },
    { VK_DOWN, 0, "MoveDown" },
    { VK_DOWN, shiftKey, "MoveDownAndModifySelection" },
    { VK_DOWN, ctrlKey | shiftKey, "MoveParagraphForwardAndModifySelection" },
    { VK_PRIOR, 0, "MovePageUp" },
    { VK_PRIOR, shiftKey, "MovePageUpAndModifySelection" },
    { VK_NEXT, 0, "MovePageDown" },
    { VK_NEXT, shiftKey, "MovePageDownAndModifySelection" },
    { VK_HOME, 0, "MoveToBeginningOfLine" },
    { VK_HOME, shiftKey, "MoveToBeginningOfLineAndModifySelection" },
    { VK_HOME, ctrlKey, "MoveToBeginningOfDocument" },
    { VK_HOME, ctrlKey | shiftKey, "MoveToBeginningOfDocumentAndModifySelection" },
    { VK_END, 0, "MoveToEndOfLine" },
    { VK_END, shiftKey, "MoveToEndOfLineAndModifySelection" },
    { VK_END, ctrlKey, "MoveToEndOfDocument" },
    { VK_END, ctrlKey | shiftKey, "MoveToEndOfDocumentAndModifySelection" },
    { VK_BACK, 0, "DeleteBackward" },
    { VK_BACK, shiftKey, "DeleteBackward" },
    { VK_BACK, ctrlKey, "DeleteWordBackward" },
    { VK_DELETE, 0, "DeleteForward" },
    { VK_DELETE, ctrlKey, "DeleteWordForward" },
    { VK_TAB, 0, "InsertTab" },
    { VK_TAB, shiftKey, "InsertBacktab" },
    { VK_RETURN, 0, "InsertNewline" },
    { VK_RETURN, ctrlKey, "InsertNewline" },
    { VK_RETURN, altKey, "InsertNewline" },
    { VK_RETURN, altKey | shiftKey, "InsertNewline" },
    { VK_RETURN, shiftKey, "InsertLineBreak" },
    { 'A', ctrlKey, "SelectAll" },
    { 'B', ctrlKey, "ToggleBold" },
    { 'I', ctrlKey, "ToggleItalic" },
    { 'U', ctrlKey, "ToggleUnderline" },
    { 'Z', ctrlKey, "Undo" },
    { 'Z', ctrlKey | shiftKey, "Redo" },
    { 'Y', ctrlKey, "Redo" },
};

static constexpr KeyPressEntry keyPressEntries[] = {
    { '\t', 0, "InsertTab" },
    { '\t', shiftKey, "InsertBacktab" },
    { '\r', 0, "InsertNewline" },
    { '\r', ctrlKey, "InsertNewline" },
    { '\r', shiftKey, "InsertLineBreak" },
    { '\r', altKey, "InsertNewline" },
    { '\r', altKey | shiftKey, "InsertNewline" },
};

static const char* interpretKeyEvent(const KeyboardEvent& event)
{
    static NeverDestroyed<HashMap<int, const char*>> keyDownCommandsMap;
    static NeverDestroyed<HashMap<int, const char*>> keyPressCommandsMap;

    if (keyDownCommandsMap.get().isEmpty()) {
        for (const auto& entry : keyDownEntries)
            keyDownCommandsMap.get().set(entry.modifiers << 16 | entry.virtualKey, entry.name);
        for (const auto& entry : keyPressEntries)
            keyPressCommandsMap.get().set(entry.modifiers << 16 | entry.charCode, entry.name);
    }

    unsigned modifiers = 0;
    if (event.shiftKey())
        modifiers |= shiftKey;
    if (event.altKey())
        modifiers |= altKey;
    if (event.ctrlKey())
        modifiers |= ctrlKey;
    if (event.metaKey())
        modifiers |= metaKey;

    if (event.type() == eventNames().keydownEvent) {
        int mapKey = modifiers << 16 | event.keyCode();
        return mapKey ? keyDownCommandsMap.get().get(mapKey) : nullptr;
    }

    int mapKey = modifiers << 16 | event.charCode();
    return mapKey ? keyPressCommandsMap.get().get(mapKey) : nullptr;
}

class Win98MiniTextCheckerClient final : public TextCheckerClient {
public:
    bool shouldEraseMarkersAfterChangeSelection(TextCheckingType) const final { return true; }
    void ignoreWordInSpellDocument(const String&) final { }
    void learnWord(const String&) final { }
    void checkSpellingOfString(StringView, int*, int*) final { }
    void checkGrammarOfString(StringView, Vector<GrammarDetail>&, int*, int*) final { }

#if USE(UNIFIED_TEXT_CHECKING)
    Vector<TextCheckingResult> checkTextOfParagraph(StringView, OptionSet<TextCheckingType>, const VisibleSelection&) final { return { }; }
#endif

    void getGuessesForWord(const String&, const String&, const VisibleSelection&, Vector<String>&) final { }
    void requestCheckingOfString(TextCheckingRequest&, const VisibleSelection&) final { }
    void requestExtendedCheckingOfString(TextCheckingRequest&, const VisibleSelection&) final { }
};

class Win98MiniEditorClient final : public EditorClient {
    WTF_MAKE_TZONE_ALLOCATED(Win98MiniEditorClient);
    WTF_OVERRIDE_DELETE_FOR_CHECKED_PTR(Win98MiniEditorClient);

private:
    bool shouldDeleteRange(const std::optional<SimpleRange>&) final { return true; }
    bool smartInsertDeleteEnabled() final { return false; }
    bool isSelectTrailingWhitespaceEnabled() const final { return false; }
    bool isContinuousSpellCheckingEnabled() final { return false; }
    void toggleContinuousSpellChecking() final { }
    bool isGrammarCheckingEnabled() final { return false; }
    void toggleGrammarChecking() final { }
    int spellCheckerDocumentTag() final { return -1; }

    bool shouldBeginEditing(const SimpleRange&) final { return true; }
    bool shouldEndEditing(const SimpleRange&) final { return true; }
    bool shouldInsertNode(Node&, const std::optional<SimpleRange>&, EditorInsertAction) final { return true; }
    bool shouldInsertText(const String&, const std::optional<SimpleRange>&, EditorInsertAction) final { return true; }
    bool shouldChangeSelectedRange(const std::optional<SimpleRange>&, const std::optional<SimpleRange>&, Affinity, bool) final { return true; }
    bool shouldApplyStyle(const StyleProperties&, const std::optional<SimpleRange>&) final { return true; }
    void didApplyStyle() final { }
    bool shouldMoveRangeAfterDelete(const SimpleRange&, const SimpleRange&) final { return false; }

    void didBeginEditing() final { }
    void respondToChangedContents() final { }
    void respondToChangedSelection(LocalFrame*) final { }
    void didEndUserTriggeredSelectionChanges() final { }
    void updateEditorStateAfterLayoutIfEditabilityChanged() final { }
    void didEndEditing() final { }
    void discardedComposition(const Document&) final { }
    void canceledComposition() final { }
    void didUpdateComposition() final { }
    void willWriteSelectionToPasteboard(const std::optional<SimpleRange>&) final { }
    void didWriteSelectionToPasteboard() final { }
    void getClientPasteboardData(const std::optional<SimpleRange>&, Vector<std::pair<String, RefPtr<SharedBuffer>>>&) final { }
    void requestCandidatesForSelection(const VisibleSelection&) final { }
    void handleAcceptedCandidateWithSoftSpaces(TextCheckingResult) final { }

    DOMPasteAccessResponse requestDOMPasteAccess(DOMPasteAccessCategory, FrameIdentifier, const String&) final { return DOMPasteAccessResponse::DeniedForGesture; }

    void registerUndoStep(UndoStep&) final { }
    void registerRedoStep(UndoStep&) final { }
    void clearUndoRedoOperations() final { }

    bool canCopyCut(LocalFrame*, bool defaultValue) const final { return defaultValue; }
    bool canPaste(LocalFrame*, bool defaultValue) const final { return defaultValue; }
    bool canUndo() const final { return false; }
    bool canRedo() const final { return false; }

    void undo() final { }
    void redo() final { }

    void handleKeyboardEvent(KeyboardEvent& event) final
    {
        RefPtr target = dynamicDowncast<Node>(event.target());
        if (!target)
            return;

        auto* frame = target->document().frame();
        if (!frame)
            return;

        auto* platformEvent = event.underlyingPlatformEvent();
        if (!platformEvent)
            return;

        if (platformEvent->windowsVirtualKeyCode() == VK_PROCESSKEY)
            return;

        if (!frame->editor().canEdit())
            return;

        if (event.type() == eventNames().keypressEvent)
            return handleKeyPress(*frame, event, *platformEvent);
        if (event.type() == eventNames().keydownEvent)
            return handleKeyDown(*frame, event);
    }

    static void handleKeyPress(LocalFrame& frame, KeyboardEvent& event, const PlatformKeyboardEvent& platformEvent)
    {
        auto commandName = String::fromLatin1(interpretKeyEvent(event));
        if (!commandName.isEmpty()) {
            frame.editor().command(commandName).execute();
            event.setDefaultHandled();
            return;
        }

        if (event.charCode() < ' ')
            return;
        if (platformEvent.controlKey() || platformEvent.altKey())
            return;

        if (frame.editor().insertText(platformEvent.text(), &event))
            event.setDefaultHandled();
    }

    static void handleKeyDown(LocalFrame& frame, KeyboardEvent& event)
    {
        auto commandName = String::fromLatin1(interpretKeyEvent(event));
        if (commandName.isEmpty())
            return;
        if (commandName == "DeleteBackward"_s || commandName == "DeleteForward"_s || commandName == "DeleteWordBackward"_s || commandName == "DeleteWordForward"_s)
            return;

        auto command = frame.editor().command(commandName);
        if (command.isTextInsertion())
            return;

        command.execute();
        event.setDefaultHandled();
    }

    void handleInputMethodKeydown(KeyboardEvent&) final { }

    void textFieldDidBeginEditing(Element&) final { }
    void textFieldDidEndEditing(Element&) final { }
    void textDidChangeInTextField(Element&) final { }
    bool doTextFieldCommandFromEvent(Element& element, KeyboardEvent* event) final
    {
        RefPtr input = dynamicDowncast<HTMLInputElement>(element);
        if (!input || !event)
            return false;

        auto* platformEvent = event->underlyingPlatformEvent();
        if (!platformEvent)
            return false;

        if (platformEvent->windowsVirtualKeyCode() == VK_BACK)
            return deleteBackwardInTextField(*input);
        if (platformEvent->windowsVirtualKeyCode() == VK_DELETE)
            return deleteForwardInTextField(*input);

        return false;
    }

    static bool deleteBackwardInTextField(HTMLInputElement& input)
    {
        String value = input.value().get();
        unsigned start = std::min(input.selectionStart(), value.length());
        unsigned end = std::min(input.selectionEnd(), value.length());
        if (!start && !end)
            return true;

        unsigned deleteStart = start == end ? start - 1 : start;
        unsigned deleteEnd = end;
        auto newValue = makeString(value.left(deleteStart), value.substring(deleteEnd));
        input.setValueForUser(newValue);
        input.setSelectionRange(deleteStart, deleteStart);
        return true;
    }

    static bool deleteForwardInTextField(HTMLInputElement& input)
    {
        String value = input.value().get();
        unsigned start = std::min(input.selectionStart(), value.length());
        unsigned end = std::min(input.selectionEnd(), value.length());
        if (start == end) {
            if (start >= value.length())
                return true;
            end = start + 1;
        }

        auto newValue = makeString(value.left(start), value.substring(end));
        input.setValueForUser(newValue);
        input.setSelectionRange(start, start);
        return true;
    }

    void textWillBeDeletedInTextField(Element&) final { }
    void textDidChangeInTextArea(Element&) final { }
    void overflowScrollPositionChanged() final { }
    void subFrameScrollPositionChanged() final { }

#if PLATFORM(IOS_FAMILY)
    void startDelayingAndCoalescingContentChangeNotifications() final { }
    void stopDelayingAndCoalescingContentChangeNotifications() final { }
    bool hasRichlyEditableSelection() final { return false; }
    int getPasteboardItemsCount() final { return 0; }
    RefPtr<DocumentFragment> documentFragmentFromDelegate(int) final { return nullptr; }
    bool performsTwoStepPaste(DocumentFragment*) final { return false; }
    void updateStringForFind(const String&) final { }
#endif

#if PLATFORM(COCOA)
    void setInsertionPasteboard(const String&) final { }
#endif

#if USE(APPKIT)
    void uppercaseWord() final { }
    void lowercaseWord() final { }
    void capitalizeWord() final { }
#endif

#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    void showSubstitutionsPanel(bool) final { }
    bool substitutionsPanelIsShowing() final { return false; }
    void toggleSmartInsertDelete() final { }
    bool isAutomaticQuoteSubstitutionEnabled() final { return false; }
    void toggleAutomaticQuoteSubstitution() final { }
    bool isAutomaticLinkDetectionEnabled() final { return false; }
    void toggleAutomaticLinkDetection() final { }
    bool isAutomaticDashSubstitutionEnabled() final { return false; }
    void toggleAutomaticDashSubstitution() final { }
    bool isAutomaticTextReplacementEnabled() final { return false; }
    void toggleAutomaticTextReplacement() final { }
    bool isAutomaticSpellingCorrectionEnabled() final { return false; }
    void toggleAutomaticSpellingCorrection() final { }
    bool isSmartListsEnabled() final { return false; }
    void toggleSmartLists() final { }
#endif

#if PLATFORM(GTK)
    bool shouldShowUnicodeMenu() final { return false; }
#endif

    TextCheckerClient* textChecker() final { return &m_textCheckerClient; }
    void updateSpellingUIWithGrammarString(const String&, const GrammarDetail&) final { }
    void updateSpellingUIWithMisspelledWord(const String&) final { }
    void showSpellingUI(bool) final { }
    bool spellingUIIsShowing() final { return false; }
    void setInputMethodState(Element*) final { }
    bool performTwoStepDrop(DocumentFragment&, const SimpleRange&, bool) final { return false; }

    Win98MiniTextCheckerClient m_textCheckerClient;
};

WTF_MAKE_TZONE_ALLOCATED_IMPL(Win98MiniEditorClient);

UniqueRef<EditorClient> createWin98MiniEditorClient()
{
    return makeUniqueRef<Win98MiniEditorClient>();
}

} // namespace WebCore
