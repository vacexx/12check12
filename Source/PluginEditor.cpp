#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
_12check12AudioProcessorEditor::_12check12AudioProcessorEditor(_12check12AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // --- TIME SLIDER ---
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "TIME", timeSlider);
    timeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    timeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20); // Zmenšil jsem text box
    timeSlider.setLookAndFeel(&customLook); // APLIKUJEME VZHLED
    addAndMakeVisible(timeSlider);

    timeLabel.setText("Time", juce::dontSendNotification);
    timeLabel.attachToComponent(&timeSlider, false);
    timeLabel.setJustificationType(juce::Justification::centredLeft);
    timeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(timeLabel); // Musíme ho pøidat ruènì

    // --- FEEDBACK SLIDER ---
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "FEEDBACK", feedbackSlider);
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    feedbackSlider.setLookAndFeel(&customLook); // APLIKUJEME VZHLED
    addAndMakeVisible(feedbackSlider);

    feedbackLabel.setText("Fdbk", juce::dontSendNotification);
    feedbackLabel.attachToComponent(&feedbackSlider, false);
    feedbackLabel.setJustificationType(juce::Justification::centred);
    feedbackLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(feedbackLabel);

    // --- MIX SLIDER ---
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "MIX", mixSlider);
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    mixSlider.setLookAndFeel(&customLook); // APLIKUJEME VZHLED
    addAndMakeVisible(mixSlider);

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, false);
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(mixLabel);

    // --- HPF SLIDER ---
    HPFAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "HPF", HPFSlider);
    HPFSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    HPFSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    HPFSlider.setLookAndFeel(&customLook); // APLIKUJEME VZHLED
    addAndMakeVisible(HPFSlider);

    HPFLabel.setText("HPF", juce::dontSendNotification);
    HPFLabel.attachToComponent(&HPFSlider, false);
    HPFLabel.setJustificationType(juce::Justification::centred);
    HPFLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(HPFLabel);

    // --- LPF SLIDER ---
    LPFAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "LPF", LPFSlider);
    LPFSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    LPFSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    LPFSlider.setLookAndFeel(&customLook); // aplikování vzhledu
    addAndMakeVisible(LPFSlider);

    LPFLabel.setText("LPF", juce::dontSendNotification);
    LPFLabel.attachToComponent(&LPFSlider, false);
    LPFLabel.setJustificationType(juce::Justification::centred);
    LPFLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(LPFLabel);

    // --- SATURATION SLIDER ---
    SaturationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "SATURATION", SaturationSlider);
    SaturationSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    SaturationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
	SaturationSlider.setLookAndFeel(&customLook); // aplikování vzhledu
    addAndMakeVisible(SaturationSlider);

    SaturationLabel.setText("Sat", juce::dontSendNotification);
    SaturationLabel.attachToComponent(&SaturationSlider, false);
    SaturationLabel.setJustificationType(juce::Justification::centred);
    SaturationLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(SaturationLabel);

	// --- TIME SYNC BUTTON ---
	syncButton.setButtonText("Sync");
	syncButton.setClickingTogglesState(true);
    
    syncButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey.darker(0.8f));
    syncButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange.darker(0.2f)); // aktivní barva
    syncButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    syncButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);

	syncButton.onClick = [this] {updateSyncButtonState();};
	addAndMakeVisible(syncButton);

    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "SYNC", syncButton);

	// --- DIVISION COMBOBOX ---
    divisionBox.addItem("1/1", 1);
    divisionBox.addItem("1/2", 2);
    divisionBox.addItem("1/4", 3);
    divisionBox.addItem("1/8", 4);
    divisionBox.addItem("1/16", 5);
    divisionBox.addItem("1/4 Triplet", 6);
    divisionBox.addItem("1/8 Triplet", 7);
    divisionBox.addItem("1/4 Dotted", 8);
    divisionBox.addItem("1/8 Dotted", 9);
    
    divisionBox.setJustificationType(juce::Justification::centred);

    // stylizace ComboBoxu
    divisionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    divisionBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey.withAlpha(0.5f));
    divisionBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    divisionBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::orange);

    addChildComponent(divisionBox); // použijeme addChild místo addAndMakeVisible, protože ze zaèátku mùže být skrytý

    divisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "DIVISION", divisionBox);

	updateSyncButtonState();

    setSize(500, 350);
}

_12check12AudioProcessorEditor::~_12check12AudioProcessorEditor()
{
    // musíme vynulovat LookAndFeel, jinak program spadne pøi zavírání
    timeSlider.setLookAndFeel(nullptr);
    feedbackSlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
    HPFSlider.setLookAndFeel(nullptr);
    LPFSlider.setLookAndFeel(nullptr);
    SaturationSlider.setLookAndFeel(nullptr);
}

//==============================================================================
void _12check12AudioProcessorEditor::paint(juce::Graphics& g)
{
    // gradient pozadí
    juce::ColourGradient bgGradient(
        juce::Colours::black,
        0, 0,
        juce::Colours::darkgrey.darker(0.8f),
        0, (float)getHeight(),
        false);

    g.setGradientFill(bgGradient);
    g.fillAll();

	// panel pro filtry
    auto bounds = getLocalBounds();
    auto bottomSection = bounds.removeFromBottom(160).reduced(10);

    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.fillRoundedRectangle(bottomSection.toFloat(), 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(bottomSection.toFloat(), 10.0f, 1.0f);

    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
}

void _12check12AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(40); // místo pro header
    area.reduce(20, 20);

    // horní sekce
    auto topSection = area.removeFromTop(area.getHeight() - 160);
    int mainKnobWidth = topSection.getWidth() / 3;

	// oblast pro synchronizaci a èasový slider 
    auto timeArea = topSection.removeFromLeft(mainKnobWidth).reduced(10);
    auto syncBtnArea = timeArea.removeFromTop(20);
    syncButton.setBounds(syncBtnArea.removeFromRight(40));
    timeSlider.setBounds(timeArea);

    // note selector
    divisionBox.setBounds(timeArea.getX(), timeArea.getY() + timeArea.getHeight() / 2 - 15, timeArea.getWidth(), 30);

    // spodní sekce
    feedbackSlider.setBounds(topSection.removeFromLeft(mainKnobWidth).reduced(10));
    mixSlider.setBounds(topSection.reduced(10));
    area.removeFromTop(30);
    auto filterKnobArea = area;
    filterKnobArea.removeFromTop(25);

    int smallKnobWidth = filterKnobArea.getWidth() / 3;
    HPFSlider.setBounds(filterKnobArea.removeFromLeft(smallKnobWidth).reduced(5));
    LPFSlider.setBounds(filterKnobArea.removeFromLeft(smallKnobWidth).reduced(5));
    SaturationSlider.setBounds(filterKnobArea.reduced(5));
}

void _12check12AudioProcessorEditor::updateSyncButtonState()
{
    bool isSyncOn = syncButton.getToggleState();

    if (isSyncOn)
    {
        // režim BPM: skryjeme manuální knob, ukážeme výbìr not
        timeSlider.setVisible(false);
        timeLabel.setVisible(false);
        divisionBox.setVisible(true);
    }
    else
    {
        // manuální režim: ukážeme knob, skryjeme výbìr not
        timeSlider.setVisible(true);
        timeLabel.setVisible(true);
        divisionBox.setVisible(false);
    }
}