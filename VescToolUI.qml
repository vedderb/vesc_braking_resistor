import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    id: mainItem
    anchors.fill: parent
    anchors.margins: 5
    
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    
    ColumnLayout {
        id: gaugeColumn
        anchors.fill: parent
        RowLayout {
            Layout.fillHeight: true
            CustomGauge {
                id: vGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 120
                minimumValue: 0
                tickmarkScale: 1
                precision: 1
                labelStep: 10
                value: 0
                unitText: "V"
                typeText: "V In\n(Not Working)"
            }
            
            CustomGauge {
                id: iGauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 120
                minimumValue: 0
                tickmarkScale: 1
                precision: 1
                labelStep: 10
                value: 0
                unitText: "A"
                typeText: "I In"
            }
        }
        
        RowLayout {
            Layout.fillHeight: true
            CustomGauge {
                id: t1Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 120
                minimumValue: -20
                tickmarkScale: 1
                precision: 1
                labelStep: 10
                value: 0
                unitText: "DegC"
                typeText: "Temp 1"
            }
            
            CustomGauge {
                id: t2Gauge
                Layout.fillWidth: true
                Layout.preferredWidth: gaugeColumn.width * 0.45
                Layout.preferredHeight: gaugeColumn.height * 0.45
                maximumValue: 120
                minimumValue: -20
                tickmarkScale: 1
                precision: 1
                labelStep: 10
                value: 0
                unitText: "DegC"
                typeText: "Temp 2"
            }
        }
        
        Text {
            Layout.fillWidth: true
            color: "White"
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 14
            text: "Output Power"
        }
        
        Slider {
            Layout.fillWidth: true
            id: pwrSlider
            
            from: 0
            to: 1000
            value: 0
            
            onValueChanged: {
                mCommands.ioBoardSetPwm(255, 0, pwrSlider.value / 1000)
            }
        }
    }
    
    Timer {
        running: true
        repeat: true
        interval: 100
        
        onTriggered: {
            mCommands.ioBoardGetAll(255)
            mCommands.ioBoardSetPwm(255, 0, pwrSlider.value / 1000)
        }
    }
    
    Connections {
        target: mCommands
        
        onIoBoardValRx: {
            vGauge.value = val.adc_1_4[0]
            iGauge.value = val.adc_1_4[1]
            t1Gauge.value = val.adc_1_4[2]
            t2Gauge.value = val.adc_1_4[3]
        }
    }
}
