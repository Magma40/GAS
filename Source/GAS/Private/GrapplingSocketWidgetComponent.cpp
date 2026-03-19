// Fill out your copyright notice in the Description page of Project Settings.
#include "GrapplingSocketWidgetComponent.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

UGrapplingSocketWidgetComponent::UGrapplingSocketWidgetComponent()
{
	SetWidgetClass(UGrapplingSocketWidget::StaticClass());
}

void UGrapplingSocketWidgetComponent::BeginPlay()
{
	Super::BeginPlay();
	InitWidget();
	//SetVisibility(false);
}

void UGrapplingSocketWidgetComponent::InitWidget()
{
	//If the widget has not been initiated at this point, force it
	Super::InitWidget();

	//Try to get the GrapplingSocketWidget
	if (IsValid(GetWidget()))
	{
		GrapplingSocketWidget = Cast<UGrapplingSocketWidget>(GetWidget());
		GrapplingSocketWidget->SetWidgetComponent(this);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:InitWidget - GetWidget() not found"), *StaticClass()->GetName());
		return;
	}
}

void UGrapplingSocketWidgetComponent::SetWidgetImageVisibility(const bool Visibility)
{
	if(!IsValid(GrapplingSocketWidget))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:SetWidgetImageVisibility - GrapplingSocketWidget not found"), *StaticClass()->GetName());
		return;
	}

	TransferSettings.bImageVisible = Visibility;
	
	//If we are not in play and widget has been initialized 
	if (!GIsPlayInEditorWorld)
	{
		GrapplingSocketWidget->RebuildWidget();
		return;
	}
	
	UImage* ImageReference = GrapplingSocketWidget->GetImage();
	if (IsValid(ImageReference))
	{
		if (Visibility) ImageReference->SetVisibility(ESlateVisibility::Visible);
		else ImageReference->SetVisibility(ESlateVisibility::Hidden);	
	}
	else
	{
			UE_LOG(LogTemp, Error, TEXT("%s:SetWidgetImageVisibility - ImageReference not found"), *StaticClass()->GetName());
			return;
	}
}

void UGrapplingSocketWidgetComponent::SetWidgetTextVisibility(const bool Visibility)
{
	if(!IsValid(GrapplingSocketWidget))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:SetWidgetTextVisibility - GrapplingSocketWidget not found"), *StaticClass()->GetName());
		return;
	}

	TransferSettings.bTextVisible = Visibility;
	
	//If we are not in play and widget has been initialized
	if (!GIsPlayInEditorWorld)
	{
		GrapplingSocketWidget->RebuildWidget();
		return;
	}

	UTextBlock* TextReference = GrapplingSocketWidget->GetText();
	if (IsValid(TextReference))
	{
		if (Visibility) TextReference->SetVisibility(ESlateVisibility::Visible);
		else TextReference->SetVisibility(ESlateVisibility::Hidden);	
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:SetWidgetTextVisibility - TextReference not found"), *StaticClass()->GetName());
		return;
	}
}

void UGrapplingSocketWidgetComponent::SetText(const FName& NewText) const
{
	if(!IsValid(GrapplingSocketWidget)) return;

	//Convert to FText to send into Text Block
	const FText LocalizedText = FText::ChangeKey(TEXT("MyNamespace"), NewText.ToString(), FText::FromName(NewText));
	GrapplingSocketWidget->SetText(LocalizedText);
}

UObject* UGrapplingSocketWidgetComponent::GetWidgetImageAUObject() const
{
	if(IsValid(GrapplingSocketWidget) && GrapplingSocketWidget->GetImage()) return Cast<UObject>(GrapplingSocketWidget->GetImage());
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:GetWidgetImageAUObject - GrapplingSocketWidget->GetImage() not found"), *StaticClass()->GetName());
		return nullptr;
	}
}

UObject* UGrapplingSocketWidgetComponent::GetWidgetTextAUObject() const
{
	if(IsValid(GrapplingSocketWidget) && GrapplingSocketWidget->GetText()) return Cast<UObject>(GrapplingSocketWidget->GetText());
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:GetWidgetTextAUObject - GrapplingSocketWidget->GetText() not found"), *StaticClass()->GetName());
		return nullptr;
	}
}


// || WIDGET CLASS || 

TSharedRef<SWidget> UGrapplingSocketWidget::RebuildWidget()
{
	Super::RebuildWidget();

	//Get the updated Settings
	const FTransferSettings TransferSettings = UpdateTransferSettings();
	
	//Construct a CanvasPanel to make a space to show widgets
	CanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel"));
	WidgetTree->RootWidget = CanvasPanel;

	//Construct an Image
	Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image"));

	//Should constructed image be visible?
	if (TransferSettings.bImageVisible) Image->SetVisibility(ESlateVisibility::Visible);
	else Image->SetVisibility(ESlateVisibility::Hidden);
	
	//Add Image as a child in the CanvasPanel
	UCanvasPanelSlot* CanvasSlotImage = CanvasPanel->AddChildToCanvas(Image);

	//Center the Image Widget 
	CanvasSlotImage->SetAnchors(FAnchors(0.5f, 0.5f));
	CanvasSlotImage->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlotImage->SetPosition(FVector2D(0.f, 0.f));
	CanvasSlotImage->SetSize(FVector2D(32.f, 32.f));

	//Construct a Text Block
	Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text"));

	//Should constructed text be visible?
	if (TransferSettings.bTextVisible) Text->SetVisibility(ESlateVisibility::Visible);
	else Text->SetVisibility(ESlateVisibility::Hidden);

	//Add Image as a child in the CanvasPanel
	UCanvasPanelSlot* CanvasSlotText = CanvasPanel->AddChildToCanvas(Text);

	//Center the Text Widget 
	CanvasSlotText->SetAnchors(FAnchors(0.15f, 0.5f));
	CanvasSlotText->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlotText->SetPosition(FVector2D(0.f, 0.f));
	CanvasSlotText->SetSize(FVector2D(32.f, 32.f));

	//Apply default text
	const FText LocalizedText = FText::ChangeKey(TEXT("MyNamespace"), DefaultTextToSay.ToString(), FText::FromName(DefaultTextToSay));
	Text->SetText(LocalizedText);
	
	//Build the widget in engine
	return Super::RebuildWidget();
}

FTransferSettings UGrapplingSocketWidget::UpdateTransferSettings() const
{
	//Construct new TransferSettings
	FTransferSettings TransferSettings = FTransferSettings();

	//If we know who owns this widget, apply its stored TransferSettings
	if (IsValid(GrapplingSocketWidgetComponent))
	{
		TransferSettings.bImageVisible = GrapplingSocketWidgetComponent->GetTransferSettings().bImageVisible;
		TransferSettings.bTextVisible = GrapplingSocketWidgetComponent->GetTransferSettings().bTextVisible;
	}
	
	return TransferSettings;
}

void UGrapplingSocketWidget::SetImageSize(const FVector2D NewSize) const
{
	if(!IsValid(Image)) return;
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Image->Slot))
	{
		//Set new image size
		CanvasSlot->SetSize(NewSize);
	}
}

void UGrapplingSocketWidget::SetText(const FText& NewText) const
{
	if(!IsValid(Text)) return;
	Text->SetText(NewText);
}
