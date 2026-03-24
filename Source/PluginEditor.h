#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class SimpleKnobStyle : public juce::LookAndFeel_V4
{
public:
    SimpleKnobStyle()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        auto center = juce::Point<float>((float)width / 2.0f, (float)height / 2.0f);
        auto rx = center.x - radius;
        auto ry = center.y - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // pozadí dráhy (tmavì šedá)
        g.setColour(juce::Colours::darkgrey.darker(0.5f));
        g.drawEllipse(rx, ry, rw, rw, 5.0f);

        // aktivní hodnota (oranžová)
        juce::Path p;
        p.addCentredArc(center.x, center.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);

        g.setColour(juce::Colours::orange);
        g.strokePath(p, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // ukazatel (bílá èárka)
        juce::Path thumb;
        thumb.addRectangle(-2.0f, -radius, 4.0f, 10.0f);
        g.setColour(juce::Colours::white);
        g.fillPath(thumb, juce::AffineTransform::rotation(angle).translated(center.x, center.y));
    }
};

//==============================================================================
class _12check12AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    _12check12AudioProcessorEditor(_12check12AudioProcessor&);
    ~_12check12AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    _12check12AudioProcessor& audioProcessor;

    SimpleKnobStyle customLook;

    juce::Slider timeSlider;
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    juce::Slider HPFSlider;
    juce::Slider LPFSlider;
    juce::Slider SaturationSlider;

    juce::Label timeLabel;
    juce::Label feedbackLabel;
    juce::Label mixLabel;
    juce::Label HPFLabel;
    juce::Label LPFLabel;
    juce::Label SaturationLabel;

	juce::TextButton syncButton;
	juce::ComboBox divisionBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> HPFAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> LPFAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> SaturationAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;

	void updateSyncButtonState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(_12check12AudioProcessorEditor)
};