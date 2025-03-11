/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12
import org.krita.flake.text 1.0

CollapsibleGroupProperty {
    propertyName: i18nc("@title:group", "Text Indent");
    propertyType: TextPropertyBase.Paragraph;
    toolTip: i18nc("@info:tooltip",
                   "Text Indent allows setting indentation at the line start. Only works when the text is wrapping.");
    searchTerms: i18nc("comma separated search terms for the text-indent property, matching is case-insensitive",
                       "text-indent");

    property alias textIndentValue: textIndentUnitCmb.dataValue;
    property alias hanging: indentHangingCkb.checked;
    property alias eachLine: eachLineCkb.checked;
    property alias textIndentUnit: textIndentUnitCmb.dataUnit;

    onPropertiesUpdated: {
        blockSignals = true;
        textIndentUnitCmb.dpi = canvasDPI;
        textIndentUnitCmb.setTextProperties(properties);
        textIndentUnitCmb.setDataValueAndUnit(properties.textIndent.length.value, properties.textIndent.length.unitType);
        hanging = properties.textIndent.hanging;
        eachLine = properties.textIndent.eachLine;
        visible = properties.textIndentState !== KoSvgTextPropertiesModel.PropertyUnset;
        blockSignals = false;
    }

    onTextIndentValueChanged: {
        if (!blockSignals) {
            properties.textIndent.length.value = textIndentValue;
        }
    }

    onTextIndentUnitChanged: {
        if (!blockSignals) {
            properties.textIndent.length.unitType = textIndentUnit;
        }
    }

    onHangingChanged: {
        if (!blockSignals) {
            properties.textIndent.hanging = hanging;
        }
    }

    onEachLineChanged: {
        if (!blockSignals) {
            properties.textIndent.eachLine = eachLine;
        }
    }

    onEnableProperty: properties.textIndentState = KoSvgTextPropertiesModel.PropertySet;

    titleItem: RowLayout {
        width: parent.width;
        height: childrenRect.height;
        spacing: columnSpacing;

        Label {
            id: propertyTitle;
            text: propertyName;
            verticalAlignment: Text.AlignVCenter
            color: sysPalette.text;
            elide: Text.ElideRight;
            Layout.maximumWidth: contentWidth;
        }

        DoubleSpinBox {
            id: textIndentSpn;
            Layout.fillWidth: true;
            from: 0;
            to: 999 * multiplier;
            onValueChanged: textIndentUnitCmb.userValue = value;
        }
        /// Note: percentage calculation in the default unitcombobox isn't great for textIndent as it assumes 100% = fontsize,
        /// While spec-wise, it's the inline length that defines percentage.
        UnitComboBox {
            id: textIndentUnitCmb;
            spinBoxControl: textIndentSpn;
            onUserValueChanged: textIndentSpn.value = userValue;
            Layout.preferredWidth: minimumUnitBoxWidth;
            Layout.maximumWidth: implicitWidth;
        }
    }

    contentItem: GridLayout {
        columns: 2
        anchors.left: parent.left
        anchors.right: parent.right
        columnSpacing: columnSpacing;

        RevertPropertyButton {
            revertState: properties.textIndentState;
            onClicked: properties.textIndentState = KoSvgTextPropertiesModel.PropertyUnset;
        }


        CheckBox {
            id: indentHangingCkb;
            text: i18nc("@option:check", "Hanging indentation")
            Layout.fillWidth: true
        }

        Item {
            width: firstColumnWidth;
            height: firstColumnWidth;
            Layout.columnSpan: 1;
        }

        CheckBox {
            id: eachLineCkb;
            text: i18nc("@option:check", "Indent after hardbreaks")
            Layout.fillWidth: true
        }

    }
}


