/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Qt includes
#include <QDebug>

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

// VTK includes
#include <vtkInteractorStyle.h>

// LookingGlass includes
#include "qSlicerLookingGlassModuleWidget.h"
#include "ui_qSlicerLookingGlassModuleWidget.h"

// LookingGlass Logic includes
#include <vtkSlicerLookingGlassLogic.h>

// LookingGlass MRML includes
#include <vtkMRMLLookingGlassViewNode.h>

// LookingGlass Widget includes
#include <qMRMLLookingGlassView.h>

// LookingGlass includes
#include "qSlicerLookingGlassModule.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerLookingGlassModuleWidgetPrivate: public Ui_qSlicerLookingGlassModuleWidget
{
public:
  qSlicerLookingGlassModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerLookingGlassModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerLookingGlassModuleWidgetPrivate::qSlicerLookingGlassModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerLookingGlassModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerLookingGlassModuleWidget::qSlicerLookingGlassModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerLookingGlassModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerLookingGlassModuleWidget::~qSlicerLookingGlassModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::setup()
{
  Q_D(qSlicerLookingGlassModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Update UI
  for (int idx = 0; idx < vtkMRMLLookingGlassViewNode::RenderingMode_Last; idx++)
  {
    d->RenderingModeComboBox->addItem(vtkMRMLLookingGlassViewNode::GetRenderingModeAsString(idx));
  }

  // Connection
  connect(d->ConnectCheckBox, SIGNAL(toggled(bool)), this, SLOT(setLookingGlassConnected(bool)));
  connect(d->RenderingEnabledCheckBox, SIGNAL(toggled(bool)), this, SLOT(setLookingGlassActive(bool)));

  // Display
  connect(d->RenderingModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onRenderingModeChanged(int)));
  connect(d->DesiredUpdateRateSlider, SIGNAL(valueChanged(double)), this, SLOT(onDesiredUpdateRateChanged(double)));
  connect(d->UpdateViewFromReferenceViewCameraButton, SIGNAL(clicked()), this, SLOT(updateViewFromReferenceViewCamera()));

  // Advanced
  connect(d->ReferenceViewNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setReferenceViewNode(vtkMRMLNode*)));
  connect(d->FocalPlanePushBackButton, SIGNAL(clicked()), this, SLOT(pushFocalPlaneBack()));
  connect(d->FocalPlanePullForwardButton, SIGNAL(clicked()), this, SLOT(pullFocalPlaneForward()));
  connect(d->UseClippingLimitsCheckBox, SIGNAL(toggled(bool)), this, SLOT(setUseClippingLimits(bool)));
  connect(d->NearClippingLimitSlider, SIGNAL(valueChanged(double)), this, SLOT(onNearClippingLimitChanged(double)));
  connect(d->FarClippingLimitSlider, SIGNAL(valueChanged(double)), this, SLOT(onFarClippingLimitChanged(double)));

  this->updateWidgetFromMRML();

  // If looking glass logic is modified it indicates that the view node may changed
  qvtkConnect(this->logic(), vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
}

//--------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  vtkMRMLLookingGlassViewNode* lgViewNode = lgLogic->GetLookingGlassViewNode();

  bool wasBlocked = d->ConnectCheckBox->blockSignals(true);
  d->ConnectCheckBox->setChecked(lgViewNode != NULL && lgViewNode->GetVisibility());
  d->ConnectCheckBox->blockSignals(wasBlocked);

  QString errorText;
  if (lgViewNode && lgViewNode->HasError())
  {
    errorText = lgViewNode->GetError().c_str();
  }
  d->ConnectionStatusLabel->setText(errorText);

  wasBlocked = d->RenderingEnabledCheckBox->blockSignals(true);
  d->RenderingEnabledCheckBox->setChecked(lgViewNode != nullptr && lgViewNode->GetActive());
  d->RenderingEnabledCheckBox->blockSignals(wasBlocked);

  wasBlocked = d->RenderingModeComboBox->blockSignals(true);
  d->RenderingModeComboBox->setCurrentIndex(lgViewNode != nullptr ? lgViewNode->GetRenderingMode() : 0);
  d->RenderingModeComboBox->blockSignals(wasBlocked);
  d->RenderingModeComboBox->setEnabled(lgViewNode != nullptr);

  wasBlocked = d->DesiredUpdateRateSlider->blockSignals(true);
  d->DesiredUpdateRateSlider->setValue(lgViewNode != nullptr ? lgViewNode->GetDesiredUpdateRate() : 0);
  d->DesiredUpdateRateSlider->setEnabled(lgViewNode != nullptr);
  d->DesiredUpdateRateSlider->blockSignals(wasBlocked);

  wasBlocked = d->ReferenceViewNodeComboBox->blockSignals(true);
  d->ReferenceViewNodeComboBox->setCurrentNode(lgViewNode != nullptr ? lgViewNode->GetReferenceViewNode() : NULL);
  d->ReferenceViewNodeComboBox->blockSignals(wasBlocked);
  d->ReferenceViewNodeComboBox->setEnabled(lgViewNode != nullptr);

  d->UpdateViewFromReferenceViewCameraButton->setEnabled(lgViewNode != nullptr
    && lgViewNode->GetReferenceViewNode() != nullptr);
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::onInteractorStyleStartInteractionEvent()
{
  Q_D(qSlicerLookingGlassModuleWidget);
  qSlicerLookingGlassModule* lgModule = dynamic_cast<qSlicerLookingGlassModule*>(this->module());
  if (!lgModule)
    {
    return;
    }
  qMRMLLookingGlassView* lgView = lgModule->viewWidget();
  if (!lgView)
    {
    return;
    }
  lgView->setReferenceViewInteractive(true);
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::onInteractorStyleEndInteractionEvent()
{
  qSlicerLookingGlassModule* lgModule = dynamic_cast<qSlicerLookingGlassModule*>(this->module());
  if (!lgModule)
    {
    return;
    }
  qMRMLLookingGlassView* lgView = lgModule->viewWidget();
  if (!lgView)
    {
    return;
    }
  lgView->setReferenceViewInteractive(false);
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::setLookingGlassConnected(bool connect)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  lgLogic->SetLookingGlassConnected(connect);
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::setLookingGlassActive(bool activate)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  lgLogic->SetLookingGlassActive(activate);
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::setReferenceViewNode(vtkMRMLNode* node)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  vtkMRMLLookingGlassViewNode* lgViewNode = lgLogic->GetLookingGlassViewNode();
  if (!lgViewNode)
    {
    return;
    }
  vtkMRMLViewNode* referenceViewNode = vtkMRMLViewNode::SafeDownCast(node);
  lgViewNode->SetAndObserveReferenceViewNode(referenceViewNode);

  // Get reference to interactor style
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
    {
    qWarning() << Q_FUNC_INFO << " failed: layout manager is not available";
    return;
    }
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(layoutManager->viewWidget(referenceViewNode));
  qMRMLThreeDView* threeDView = threeDWidget->threeDView();
  vtkInteractorStyle* interactor = vtkInteractorStyle::SafeDownCast(threeDView->interactorStyle());
  qvtkReconnect(interactor, vtkCommand::StartInteractionEvent, this, SLOT(onInteractorStyleStartInteractionEvent()));
  qvtkReconnect(interactor, vtkCommand::EndInteractionEvent, this, SLOT(onInteractorStyleEndInteractionEvent()));
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::updateViewFromReferenceViewCamera()
{
  Q_D(qSlicerLookingGlassModuleWidget);
  qSlicerLookingGlassModule* lgModule = dynamic_cast<qSlicerLookingGlassModule*>(this->module());
  if (!lgModule)
    {
    return;
    }
  qMRMLLookingGlassView* lgView = lgModule->viewWidget();
  if (!lgView)
    {
    return;
    }
  lgView->updateViewFromReferenceViewCamera();
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::onRenderingModeChanged(int mode)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  vtkMRMLLookingGlassViewNode* lgViewNode = lgLogic->GetLookingGlassViewNode();
  if (lgViewNode)
  {
    lgViewNode->SetRenderingMode(mode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::onDesiredUpdateRateChanged(double fps)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  vtkMRMLLookingGlassViewNode* lgViewNode = lgLogic->GetLookingGlassViewNode();
  if (lgViewNode)
    {
    lgViewNode->SetDesiredUpdateRate(fps);
    }
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::setUseClippingLimits(bool activate)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  vtkMRMLLookingGlassViewNode* lgViewNode = lgLogic->GetLookingGlassViewNode();
  if (lgViewNode)
    {
    lgViewNode->SetUseClippingLimits(activate);
    }

  // Update UI
  // TODO Update UI by observing view node
  d->NearClippingLimitSlider->setEnabled(activate);
  d->FarClippingLimitSlider->setEnabled(activate);
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::onNearClippingLimitChanged(double limit)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  vtkMRMLLookingGlassViewNode* lgViewNode = lgLogic->GetLookingGlassViewNode();
  if (lgViewNode)
    {
    lgViewNode->SetNearClippingLimit(limit);
    }
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::onFarClippingLimitChanged(double limit)
{
  Q_D(qSlicerLookingGlassModuleWidget);
  vtkSlicerLookingGlassLogic* lgLogic = vtkSlicerLookingGlassLogic::SafeDownCast(this->logic());
  vtkMRMLLookingGlassViewNode* lgViewNode = lgLogic->GetLookingGlassViewNode();
  if (lgViewNode)
    {
    lgViewNode->SetFarClippingLimit(limit);
    }
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::pullFocalPlaneForward()
{
  qSlicerLookingGlassModule* lgModule = dynamic_cast<qSlicerLookingGlassModule*>(this->module());
  if (!lgModule)
    {
    return;
    }
  qMRMLLookingGlassView* lgView = lgModule->viewWidget();
  if (!lgView)
    {
    return;
    }
  lgView->pullFocalPlaneForward();
}

//-----------------------------------------------------------------------------
void qSlicerLookingGlassModuleWidget::pushFocalPlaneBack()
{
  qSlicerLookingGlassModule* lgModule = dynamic_cast<qSlicerLookingGlassModule*>(this->module());
  if (!lgModule)
    {
    return;
    }
  qMRMLLookingGlassView* lgView = lgModule->viewWidget();
  if (!lgView)
    {
    return;
    }
  lgView->pushFocalPlaneBack();
}
