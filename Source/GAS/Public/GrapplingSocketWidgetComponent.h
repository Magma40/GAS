// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GrapplingSocketWidgetComponent.generated.h"

class UCanvasPanel;
class UImage;
class UTextBlock;
class UGrapplingSocketWidget;

USTRUCT()
struct FTransferSettings
{
	GENERATED_BODY();
	
	UPROPERTY() bool bImageVisible = true;
	UPROPERTY() bool bTextVisible = true;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAS_API UGrapplingSocketWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UGrapplingSocketWidgetComponent();
	virtual void BeginPlay() override;

	//Function to initialize the widget and its reference
	virtual void InitWidget() override;

	//Sets a new visibility for this Widget Component
	void SetWidgetVisibility(const bool Visibility) { SetVisibility(Visibility, false);}

	//Sets a new visibility for this Widget Image Object
	void SetWidgetImageVisibility(const bool Visibility);

	//Sets a new visibility for this Widget Text Object
	void SetWidgetTextVisibility(bool Visibility);

	//Set a new text for the text block
	void SetText(const FName& NewText) const;

	//Get Widget Image as a UObject, this is made so you don't need to unnecessary includes or forwards outside this script
	UObject* GetWidgetImageAUObject() const;

	//Get Widget Text as a UObject, this is made so you don't need to unnecessary includes or forwards outside this script
	UObject* GetWidgetTextAUObject() const;

	//Gets the Transfer Settings
	FTransferSettings& GetTransferSettings()  { return TransferSettings; }
	
private:

	//GrapplingSocketWidget Reference
	UPROPERTY() UGrapplingSocketWidget* GrapplingSocketWidget = nullptr;

	//Cached Transfer Settings for updating the widget
	FTransferSettings TransferSettings = FTransferSettings();
};

// || WIDGET CLASS || 

UCLASS()
class GAS_API UGrapplingSocketWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	//Get Image Reference
	UImage* GetImage() const { return Image; }

	//Get CanvasPanel Reference
	UCanvasPanel* GetCanvasPanel() const { return CanvasPanel; }

	//Get TextBlock Reference
	UTextBlock* GetText() const { return Text; }

	//Set a new size for the image
	void SetImageSize(FVector2D NewSize) const;

	//Set a new text for the text block
	void SetText(const FText& NewText) const;

	//Sets the GrapplingSocketWidgetComponent reference
	void SetWidgetComponent(UGrapplingSocketWidgetComponent* NewWidgetComponent) { GrapplingSocketWidgetComponent = NewWidgetComponent; }

	//Custom Widget Builder
	virtual TSharedRef<SWidget> RebuildWidget() override;

	//Updates the variables on what get shown or not
	FTransferSettings UpdateTransferSettings() const;
	
private:
	//Reference to GrapplingSocketWidgetComponent(who owns this UserWidget), this is a solution because I cant get component from UserWidget
	UPROPERTY() UGrapplingSocketWidgetComponent* GrapplingSocketWidgetComponent = nullptr;
	
	//Canvas Reference
	UPROPERTY() UCanvasPanel* CanvasPanel = nullptr;

	//Image Reference
	UPROPERTY() UImage* Image = nullptr;

	//Text Block Reference
	UPROPERTY() UTextBlock* Text = nullptr;

	//Default Text for Text Block Reference
	UPROPERTY() FName  DefaultTextToSay = TEXT("Press Left Click To Grapple!");
};
