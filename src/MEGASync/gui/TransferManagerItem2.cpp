#include "TransferManagerItem2.h"
#include "ui_TransferManagerItem.h"
#include "megaapi.h"
#include "control/Utilities.h"
#include "Preferences.h"
#include "MegaApplication.h"

#include <QMouseEvent>

using namespace mega;

TransferManagerItem2::TransferManagerItem2(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::TransferManagerItem),
    mPreferences(Preferences::instance()),
    mMegaApi(nullptr),
    mTransferTag(0),
    mIsPaused(false),
    mIsFinished(false),
    mRow(0)
{
    mUi->setupUi(this);
    mAreDlPaused = mPreferences->getDownloadsPaused();
    mAreUlPaused = mPreferences->getUploadsPaused();

    // Connect to pause state change signal
    QObject::connect((MegaApplication *)qApp, &MegaApplication::pauseStateChanged,
                      this, &TransferManagerItem2::onPauseStateChanged);

//    mStatusWidgetsByType[MegaTransfer::TYPE_DOWNLOAD] = mUi->pActive;
//    mStatusWidgetsByType[MegaTransfer::TYPE_UPLOAD] = mUi->pActive;
//    mStatusWidgetsByType[MegaTransfer::TYPE_LOCAL_HTTP_DOWNLOAD] = mUi->pActive;
//    mStatusWidgetsByType[MegaTransfer::TYPE_LOCAL_TCP_DOWNLOAD] = mUi->pActive;

//    mStatusWidgetsByState[MegaTransfer::STATE_ACTIVE] = mUi->pActive;
//    mStatusWidgetsByState[MegaTransfer::STATE_QUEUED] = mUi->pQueued;
//    mStatusWidgetsByState[MegaTransfer::STATE_FAILED] = mUi->pFailed;
//    mStatusWidgetsByState[MegaTransfer::STATE_CANCELLED] = mUi->pCanceled;
//    mStatusWidgetsByState[MegaTransfer::STATE_COMPLETED] = mUi->pCompleted;
//    mStatusWidgetsByState[MegaTransfer::STATE_COMPLETING] = mUi->pCompleting;

}

void TransferManagerItem2::updateUi(QExplicitlySharedDataPointer<TransferData> data, const int row)
{
    // Update members
    mRow = row;
    mMegaApi = data->mMegaApi;
    mTransferTag = data->mTag;
    mIsPaused = false;
    mIsFinished = false;

    QString statusString;
    bool isGlobalPaused(false);
    QString timeString;
    QString speedString;
    QString pauseResumeTooltip;
    QString cancelClearTooltip;
    bool showTPauseResume(true);
    bool showTCancelClear(true);

    // File type icon
    QIcon icon (Utilities::getCachedPixmap(
                    Utilities::getExtensionPixmapName(data->mFilename,
                                                      QLatin1Literal(":/images/small_"))));
    mUi->tFileType->setIcon(icon);
    // Amount transfered
    mUi->lDone->setText(Utilities::getSizeString(data->mTransferredBytes));
    // Total size
    mUi->lTotal->setText(Utilities::getSizeString(data->mTotalSize));
    // Progress bar
    int permil = data->mState == MegaTransfer::STATE_COMPLETED
                  || data->mState == MegaTransfer::STATE_COMPLETING ?
                     1000
                   : data->mTotalSize > 0 ?
                         (1000 * data->mTransferredBytes) / data->mTotalSize
                       : 0;
    mUi->pbTransfer->setValue(permil);
    // File name
    mUi->lTransferName->setText(mUi->lTransferName->fontMetrics()
                                .elidedText(data->mFilename, Qt::ElideMiddle,
                                            mUi->wName->width()-24));
    mUi->lTransferName->setToolTip(data->mFilename);

    // Transfer type icon + text
    switch (data->mType)
    {
        case MegaTransfer::TYPE_DOWNLOAD:
        case MegaTransfer::TYPE_LOCAL_TCP_DOWNLOAD:
        {
            icon = Utilities::getCachedPixmap(QString::fromUtf8(":/images/download_item_ico.png"));
            statusString = QObject::tr("Downloading");
            isGlobalPaused = mAreDlPaused;
            break;
        }
        case MegaTransfer::TYPE_UPLOAD:
        {
            icon = Utilities::getCachedPixmap(QString::fromUtf8(":/images/upload_item_ico.png"));
            statusString = QObject::tr("Uploading");
            isGlobalPaused = mAreUlPaused;
            break;
        }
    }
    mUi->bSpeed->setIcon(icon);

    // Set values according to transfer state
    switch (data->mState)
    {
        case MegaTransfer::STATE_ACTIVE:
        {
            long long httpSpeed(data->mMegaApi->getCurrentSpeed(data->mType));
            switch (data->mType)
            {
                case MegaTransfer::TYPE_DOWNLOAD:
                case MegaTransfer::TYPE_LOCAL_TCP_DOWNLOAD:
                {
                    statusString = QObject::tr("Downloading");
                    break;
                }
                case MegaTransfer::TYPE_UPLOAD:
                {
                    statusString = QObject::tr("Uploading");
                    break;
                }
            }
            // Override speed if http speed is lower
            timeString = (httpSpeed == 0 || data->mSpeed == 0) ?
                             timeString
                           :  Utilities::getTimeString(data->mRemainingTime);
            speedString = Utilities::getSizeString(std::min(data->mSpeed, httpSpeed))
                          + QLatin1Literal("/s");
            icon = Utilities::getCachedPixmap(QLatin1Literal(":images/ico_pause_transfers_state.png"));
            pauseResumeTooltip = QObject::tr("Pause transfer");
            cancelClearTooltip = QObject::tr("Cancel transfer");
            mUi->sStatus->setCurrentWidget(mUi->pActive);
            break;
        }
        case MegaTransfer::STATE_PAUSED:
        {
            icon = Utilities::getCachedPixmap(QLatin1Literal(":images/ico_resume_transfers_state.png"));
            pauseResumeTooltip = QObject::tr("Resume transfer");
            cancelClearTooltip = QObject::tr("Cancel transfer");
            mUi->sStatus->setCurrentWidget(mUi->pPaused);
            mIsPaused = true;
            break;
        }
        case MegaTransfer::STATE_QUEUED:
        {
            icon = Utilities::getCachedPixmap(QLatin1Literal(":images/ico_pause_transfers_state.png"));
            pauseResumeTooltip = QObject::tr("Pause transfer");
            cancelClearTooltip = QObject::tr("Cancel transfer");
            mUi->sStatus->setCurrentWidget(mUi->pQueued);
            break;
        }
        case MegaTransfer::STATE_CANCELLED:
        {
            statusString = QObject::tr("Canceled");
            cancelClearTooltip = QObject::tr("Clear transfer");
            showTPauseResume = false;
            mUi->sStatus->setCurrentWidget(mUi->pActive);
            mIsFinished = true;
            break;
        }
        case MegaTransfer::STATE_COMPLETING:
        {
            statusString = QObject::tr("Completing");
            showTPauseResume = false;
            showTCancelClear = false;
            mUi->sStatus->setCurrentWidget(mUi->pActive);
            break;
        }
        case MegaTransfer::STATE_FAILED:
        {
            mUi->sStatus->setCurrentWidget(mUi->pFailed);
            cancelClearTooltip = QObject::tr("Clear transfer");
            showTPauseResume = false;
            mIsFinished = true;
            mUi->tRetry->setToolTip(tr(MegaError::getErrorString(data->mErrorCode)));
            break;
        }
        case MegaTransfer::STATE_RETRYING:
        {
            statusString = QObject::tr("Retrying");
            icon = Utilities::getCachedPixmap(QLatin1Literal(":images/ico_pause_transfers_state.png"));
            pauseResumeTooltip = QObject::tr("Pause transfer");
            cancelClearTooltip = QObject::tr("Cancel transfer");
            mUi->sStatus->setCurrentWidget(mUi->pActive);
            break;
        }
        case MegaTransfer::STATE_COMPLETED:
        {
            statusString = QObject::tr("Completed");
            cancelClearTooltip = QObject::tr("Clear transfer");
            showTPauseResume = false;
            mUi->sStatus->setCurrentWidget(mUi->pActive);
            mIsFinished = true;
            break;
        }
    }

    // Override if global/ul/dl transfers are paused
    if (isGlobalPaused)
    {
        switch (data->mState)
        {
            case MegaTransfer::STATE_ACTIVE:
            case MegaTransfer::STATE_QUEUED:
            case MegaTransfer::STATE_RETRYING:
            {
                mUi->sStatus->setCurrentWidget(mUi->pPaused);
                speedString = QString();
                timeString = QString();
            }
            default:
            {
                showTPauseResume = false;
                break;
            }
        }
    }

    // Status string
    mUi->lStatus->setText(statusString);
    // Speed
    mUi->bSpeed->setText(speedString);
    // Remaining time
    mUi->lTime->setText(timeString);
    // Pause/Resume button
    if (showTPauseResume)
    {
        mUi->tPauseResumeTransfer->setIcon(icon);
        mUi->tPauseResumeTransfer->setToolTip(pauseResumeTooltip);
    }
    mUi->tPauseResumeTransfer->setVisible(showTPauseResume);

    // Cancel/Clear Button
    if (showTCancelClear)
    {
        mUi->tCancelClearTransfer->setToolTip(cancelClearTooltip);
    }
    mUi->tCancelClearTransfer->setVisible(showTCancelClear);

    update();
}

void TransferManagerItem2::on_tPauseResumeTransfer_clicked()
{
    mMegaApi->pauseTransferByTag(mTransferTag, !mIsPaused);
}

void TransferManagerItem2::on_tCancelClearTransfer_clicked()
{
    // Clear
    emit clearTransfers(mRow, 1);
}

void TransferManagerItem2::forwardMouseEvent(QMouseEvent *me)
{
    auto w (childAt(me->pos() - pos()));
    if (w)
    {
        auto t (qobject_cast<QToolButton*>(w));
        if (t)
        {
            t->click();
        }
    }
}

void TransferManagerItem2::on_tRetry_clicked()
{
    emit retryTransfer(mTransferTag);
}

void TransferManagerItem2::onPauseStateChanged()
{
    mAreDlPaused = mPreferences->getDownloadsPaused();
    mAreUlPaused = mPreferences->getUploadsPaused();
}
