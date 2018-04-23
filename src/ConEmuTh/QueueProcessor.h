
/*
Copyright (c) 2010-present Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#ifndef TRY
	#define TRY __try
	#define CATCH __except(EXCEPTION_EXECUTE_HANDLER)
#endif

//#ifndef sizeofarray
//	#define sizeofarray(x) (sizeof(x)/sizeof(*x))
//#endif

enum ProcessingPriority
{
	ePriorityHighest = 0, // Этот элемент нужно обрабатывать сразу
	ePriorityAboveNormal,
	ePriorityNormal,
	ePriorityBelowNormal,
	ePriorityLowest,
};

enum ProcessingStatus
{
	eItemEmpty = 0,
	eItemPassed, // почти то-же что и eItemEmpty, но обработан
	eItemFailed,
	eItemRequest,
	eItemReady,
	eItemProcessing, // NOW!
};

#include "../common/MSectionSimple.h"
#include "../common/WThreads.h"

template<class T>
class CQueueProcessor
{
	private:
		// typedefs
		struct ProcessingItem
		{
			int      Status;  // enum ProcessingStatus
			int      Priority;// enum ProcessingPriority
			HRESULT  Result;  // результат из ProcessItem
			bool     Synch;   // Устанавливается в true при синхронной обработке
			//HANDLE Ready;   // Event, может использоваться для ожидания завершения
			DWORD    Start;   // Tick старта обработки
			// Если требуется останов обработки только активного элемента -
			// появился другой, высоко-приоритетный, который ожидает обработки
			// Эту функцию могут вызывать потомки, для ускорения реакции на
			// высоко-приоритетные запросы. Удаление активного элемента из
			// очереди не произойдет - он потом будет обработан повторно.
			bool     StopRequested;
			// Информация об элементе
			T        Item;    // копия памяти из RequestItem
			LONG_PTR lParam;  // для внутренних нужд потомков
		};
		// Очередь обработки
		MSectionSimple* mpcs_Queue;
		int mn_Count, mn_MaxCount;
		int mn_WaitingCount;
		HANDLE mh_Waiting; // Event для передергивания нити обработчика
		ProcessingItem** mpp_Queue; // **, чтобы не беспокоиться об активном элементе
		HANDLE mh_Queue; DWORD mn_QueueId;
		HANDLE mh_Alive; DWORD mn_AliveTick;
		// Активный элемент (обрабатывается сейчас)
		ProcessingItem* mp_Active;  // !!! В NULL не сбрасывать !!!
		ProcessingItem* mp_SynchRequest; // Если сейчас есть "синхронный" запрос

		// Запрос на Terminate нити обработки
		bool mb_TerminateRequested;

	private:
		// Выделить память на элементы очереди. Если элементы уже выделены -
		// очистка существующих не производится. Добавляются новые (пустые)
		// ячейки, если anInitialCount превышает mn_MaxCount
		// Наличие свободных ячеек функция не проверяет.
		// СЕКЦИЯ УЖЕ ДОЛЖНА БЫТЬ ЗАБЛОКИРОВАНА
		void InitializeQueue(int anInitialCount)
		{
			if (anInitialCount < mn_MaxCount && mpp_Queue)
			{
				// Уже выделено достаточно элементов
				return;
			}

			ProcessingItem** pp = (ProcessingItem**)calloc(anInitialCount,sizeof(ProcessingItem*));

			if (mpp_Queue && mn_Count)
			{
				_ASSERTE(mn_Count <= mn_MaxCount);
				memmove(pp, mpp_Queue, mn_Count*sizeof(ProcessingItem*));
			}

			if (mpp_Queue)
				free(mpp_Queue);
			mpp_Queue = pp;

			for(int i = mn_MaxCount; i < anInitialCount; i++)
			{
				mpp_Queue[i] = (ProcessingItem*)calloc(1,sizeof(ProcessingItem));
			}

			mn_MaxCount = anInitialCount;
			// Done
		};
	private:
		// Полная очистка очереди. НЕ допускается во время жизни нити!
		void ClearQueue()
		{
			_ASSERTE(mh_Queue==NULL || WaitForSingleObject(mh_Queue,0)==WAIT_OBJECT_0);

			if (mpp_Queue)
			{
				_ASSERTE(eItemEmpty == 0);

				for(int i = 0; i < mn_MaxCount; i++)
				{
					if (mpp_Queue[i])
					{
						if (mpp_Queue[i]->Status != eItemEmpty && mpp_Queue[i]->Status != eItemPassed)
						{
							FreeItem(mpp_Queue[i]->Item);
						}

						free(mpp_Queue[i]);
						mpp_Queue[i] = NULL;
					}
				}

				free(mpp_Queue);
				mpp_Queue = NULL;
			}
		};
	private:
		// Warning!!! Должна вызываться ВНУТРИ CriticalSection
		void CheckWaitingCount()
		{
			int nWaiting = 0;

			for(int i = 0; i < mn_Count; i++)
			{
				if (mpp_Queue[i]->Status == eItemRequest)
					nWaiting ++;
			}

			mn_WaitingCount = nWaiting;

			if (mn_WaitingCount > 0)
				SetEvent(mh_Waiting);
			else
				ResetEvent(mh_Waiting);
		}
		// Найти в очереди элемент (если его уже запросили) или добавить в очередь новый
		ProcessingItem* FindOrCreate(const T& pItem, LONG_PTR lParam, bool Synch, ProcessingPriority Priority)
		{
			if (GetCurrentThreadId() == mn_QueueId)
			{
				_ASSERTE(GetCurrentThreadId() != mn_QueueId);
			}

			int i;
			ProcessingItem* p = NULL;
			MSectionLockSimple CS;
			CS.Lock(mpcs_Queue);

			if (!mpp_Queue || !mn_MaxCount)
			{
				// Очередь еще не была инициализирована
				InitializeQueue(64);
			}
			else
			{
				// Если такой запрос уже сделан - просто обновить его приоритет/флаги?
				for(i = 0; i < mn_Count; i++)
				{
					//if (mpp_Queue[i]->Status != eItemEmpty
					//	&& mpp_Queue[i]->Status != eItemPassed
					//	&& mpp_Queue[i]->Status != eItemFailed)
					if (mpp_Queue[i]->Status == eItemRequest)
					{
						if (IsEqual(pItem, lParam, (mpp_Queue[i]->Item), mpp_Queue[i]->lParam))
						{
							// Можно чуть подправить параметры
							mpp_Queue[i]->Priority = Priority;
							mpp_Queue[i]->lParam = lParam;
							mpp_Queue[i]->Synch = Synch;
							ApplyChanges(mpp_Queue[i]->Item, pItem);
							//SetEvent(mh_Waiting);
							CheckWaitingCount();
							_ASSERTE(mpp_Queue[i]->Status != eItemEmpty);
							CS.Unlock();
							//if (mpp_Queue[i]->Status == eItemRequest) -- Теоретически может быть уже выполнен, но здесь - не волнует
							return mpp_Queue[i];
						}
					}
					else if (!p && (mpp_Queue[i]->Status == eItemEmpty || mpp_Queue[i]->Status == eItemPassed))
					{
						p = mpp_Queue[i]; // пустой элемент, если не нашли "тот же"
					}
				}

				// Если "пустую" ячейку не нашли, и свободных не осталось
				if (p == NULL && mn_Count >= mn_MaxCount)
				{
					_ASSERTE(mn_Count == mn_MaxCount);
					InitializeQueue(mn_MaxCount+64);
				}
			}

			// Новая ячейка?
			if (!p)
			{
				if (mn_Count < mn_MaxCount)
				{
					p = mpp_Queue[mn_Count++];
				}

				_ASSERTE(p != NULL);
			}

			// Копируем данные
			if (p)
			{
				p->Priority = Priority;
				p->lParam = lParam;
				p->Result = S_OK;
				p->Start = 0;
				p->StopRequested = false;
				p->Synch = Synch;
				CopyItem(&pItem, &p->Item);
				// Отпускаем
				_ASSERTE(p->Status == eItemEmpty || p->Status == eItemPassed);
				p->Status = eItemRequest;
				mn_WaitingCount ++;
				SetEvent(mh_Waiting);
				_ASSERTE(p->Status != eItemEmpty);
			}

			CheckWaitingCount();
			CS.Unlock();
			return p;
		};
	public:
		CQueueProcessor()
		{
			mpcs_Queue = new MSectionSimple(true);
			mn_Count = mn_MaxCount = mn_WaitingCount = 0;
			mh_Waiting = NULL;
			mpp_Queue = NULL;
			mh_Queue = NULL; mn_QueueId = 0;
			mp_Active = mp_SynchRequest = NULL;
			mh_Alive = NULL; mn_AliveTick = NULL;
			mb_TerminateRequested = false;
		};
		virtual ~CQueueProcessor()
		{
			Terminate(250);

			if (mh_Queue)
			{
				CloseHandle(mh_Queue); mh_Queue = NULL;
			}

			if (mh_Alive)
			{
				CloseHandle(mh_Alive);
				mh_Alive = NULL;
			}

			SafeDelete(mpcs_Queue);
			ClearQueue();

			if (mh_Waiting)
			{
				CloseHandle(mh_Waiting);
				mh_Waiting = NULL;
			}
		};

	public:
		// Запрос на завершение очереди
		void RequestTerminate()
		{
			mb_TerminateRequested = true;
			SetEvent(mh_Waiting);
		};

	public:
		// Немедленное завершение очереди. Очереди дается nTimeout на корректный останов.
		void Terminate(DWORD nTimeout = INFINITE)
		{
			if (!mh_Queue)
			{
				// уже
				return;
			}

			// Выставить флаг
			RequestTerminate();

			// Недопустимо
			if (GetCurrentThreadId() == mn_QueueId)
			{
				_ASSERTE(GetCurrentThreadId() != mn_QueueId);
				return;
			}

			// Дождаться завершения
			DWORD nWait = WaitForSingleObject(mh_Queue, nTimeout);

			if (nWait != WAIT_OBJECT_0)
			{
				_ASSERTE(nWait == WAIT_OBJECT_0);
				apiTerminateThread(mh_Queue, 100);
			}
		};
	public:
		// Если понадобится определить "живость" нити обработки
		bool IsAlive()
		{
			if (!mh_Alive)
				return false;

			ResetEvent(mh_Alive);
			DWORD nDelta = (GetTickCount() - mn_AliveTick);

			if (nDelta < 100)
				return true;

			//TODO: Возможно придется поменять логику, посмотрим
			if (WaitForSingleObject(mh_Alive, 500) == WAIT_OBJECT_0)
				return true;

			return false;
		}
	public:
		// ВНИМАНИЕ!!! Содержимое pItem КОПИРУЕТСЯ во внутренний буфер.
		// При вызове ProcessItem приходит указатель на новый элемент из внутреннего буфера.
		// Если нужно передать дополнительный указатель - пользоваться lParam
		// Если Synch==true - функция ожидает завершения обработки элемента, а Priority устанавливается в ePriorityHighest
		// Возврат - при (Synch==true) результат ProcessItem(...)
		//           при (Synch==false) - S_FALSE, если еще в очереди, иначе - результат ProcessItem(...)
		// !!! При синхронном запросе - результат возвращается через pItem.
		HRESULT RequestItem(bool Synch, ProcessingPriority Priority, const T pItem, LONG_PTR lParam)
		{
			HRESULT hr = CheckThread();

			if (FAILED(hr))
				return hr;

			ProcessingItem* p = FindOrCreate(pItem, lParam, Synch, Synch ? ePriorityHighest : Priority);

			if (!p)
			{
				_ASSERTE(p!=NULL);
				return E_UNEXPECTED;
			}

			_ASSERTE(p->Status != eItemEmpty);

			// Если хотят результат "сейчас"
			if (Synch)
			{
				// Если уже обработан - сразу вернем результат.
				if (p->Status == eItemPassed)
				{
					// Уже обработан асинхронно
					return p->Result;
				}

				// Установить указатель на "синхронный" запрос
				mp_SynchRequest = p;

				// Проверим, есть ли активный элемент?
				if (mp_Active && mp_Active != p)
				{
					// Сейчас в нити обработки обрабатывается другой элемент
					// Желательно остановить или отложить (если возможно) его обработку
					mp_Active->StopRequested = true;
				}

				//// Для удобства - назначить Event
				//if (p->Ready)
				//	ResetEvent(p->Ready);
				//else
				//	p->Ready = CreateEvent(NULL, FALSE, FALSE, NULL);
				// Ожидаем завершения
				DWORD nWait = -1;

				//HANDLE hEvents[2] = {p->Ready, mh_Queue};
				while((nWait = WaitForSingleObject(mh_Queue,10)) == WAIT_TIMEOUT)
				{
					//nWait = WaitForMultipleObjects(2, hEvents, FALSE, 25);
					//if (nWait == (WAIT_OBJECT_0 + 1))
					//{
					//	// Нить обработки была завершена
					//	return E_ABORT;
					//}
					//if (p->Status == eItemPassed)
					//{
					//	// Уже обработан асинхронно
					//	return p->Result;
					//}
					if (p->Status == eItemPassed
					    || (p->Status == eItemFailed || p->Status == eItemReady))
					{
						if (mp_SynchRequest == p)
						{
							mp_SynchRequest = NULL;
						}

						//CloseHandle(p->Ready);
						//p->Ready = NULL;
						//// Вернуть результат обработки (данные)
						//CopyItem(&p->Item, &pItem);
						//// И сброс нашей внутренней ячейки
						//memset(&p->Item, 0, sizeof(p->Item));
						//p->Status = eItemEmpty;
						// Возврат значения
						return p->Result;
					}

					//if (nWait != WAIT_TIMEOUT)
					//{
					//	_ASSERTE(nWait == WAIT_TIMEOUT);
					//	return E_ABORT;
					//}
				}

				// Нить обработки была завершена
				return E_ABORT;
			}

			// При готовности - результат возвращается через OnItemReady / OnItemFailed
			return S_FALSE;
			//if (p->Status == eItemRequest || p->Status == eItemProcessing)
			//	return S_FALSE; // в процессе
			//
			//if (p->Status != eItemFailed && p->Status != eItemReady)
			//{
			//	_ASSERTE(p->Status == eItemFailed || p->Status == eItemReady);
			//	return E_UNEXPECTED;
			//}
			//// Вернуть результат обработки (данные)
			//CopyItem(&p->Item, pItem);
			//// И сброс нашей внутренней ячейки
			//memset(&p->Item, 0, sizeof(p->Item));
			//p->Status = eItemEmpty;
			//// Возврат значения
			//return p->Result;
		};

		// Перед помещением более актуальных элементов в очередь можно
		// уменьшить приоритет текущих элементов на количество ступеней
		// альтернатива функции ResetQueue
		void DiscountPriority(UINT nSteps = 1)
		{
			if (mpp_Queue)
			{
				_ASSERTE(eItemEmpty == 0);
				// Для получения элемента - нужно заблокировать секцию
				MSectionLockSimple CS;
				CS.Lock(mpcs_Queue);

				for(int i = 0; i < mn_MaxCount; i++)
				{
					if (mpp_Queue[i])
					{
						if (mpp_Queue[i]->Status != eItemEmpty
						        && mpp_Queue[i]->Status != eItemPassed
						        && mpp_Queue[i]->Status != eItemFailed)
						{
							int nNew = std::min(ePriorityLowest, (mpp_Queue[i]->Priority+nSteps));
							mpp_Queue[i]->Priority = nNew;
						}
					}
				}

				CheckWaitingCount();
				// Больше не требуется
				CS.Unlock();
			}
		};

		// Очищаются все ячейки, с приоритетом равным или ниже указанного
		// альтернатива функции DiscountPriority
		void ResetQueue(ProcessingPriority priority = ePriorityHighest)
		{
			// -- Сначала - останов -- не будем
			//Terminate();

			// сброс ячеек, удовлетворяющих условию
			if (mpp_Queue)
			{
				_ASSERTE(eItemEmpty == 0);
				// Для получения следующего элемента - нужно заблокировать секцию
				MSectionLockSimple CS;
				CS.Lock(mpcs_Queue);

				for (int i = 0; i < mn_MaxCount; i++)
				{
					if (mpp_Queue[i])
					{
						if (mpp_Queue[i]->Status != eItemEmpty
						        && mpp_Queue[i]->Status != eItemPassed
						        && mpp_Queue[i]->Status != eItemProcessing
						        && mpp_Queue[i]->Priority >= priority
						  )
						{
							FreeItem(&mpp_Queue[i]->Item);
						}

						memset(mpp_Queue[i], 0, sizeof(ProcessingItem));
					}
				}

				CheckWaitingCount();
				// Больше не требуется
				CS.Unlock();
			}

			//mn_Count = 0; -- !!! нельзя !!!
		};

		// Если требуется останов обработки только активного элемента -
		// появился другой, высоко-приоритетный, который ожидает обработки
		// Эту функцию могут вызывать потомки, для ускорения реакции на
		// высоко-приоритетные запросы. Удаление активного элемента из
		// очереди не произойдет - он потом будет обработан повторно.
		virtual bool IsStopRequested(const T& pItem)
		{
			_ASSERTE(mp_Active && pItem == mp_Active->Item);

			if (!mp_Active)
				return false;

			return mp_Active->StopRequested;
		};

	protected:
		/* *** Подлежит! обязательному переопределению в потомках *** */

		// Обработка элемента. Функция должна возвращать:
		// S_OK    - Элемент успешно обработан, будет установлен статус eItemReady
		// S_FALSE - ошибка обработки, будет установлен статус eItemFailed
		// FAILED()- статус eItemFailed И нить обработчика будет ЗАВЕРШЕНА
		virtual HRESULT ProcessItem(T& pItem, LONG_PTR lParam)
		{
			return E_NOINTERFACE;
		};

	protected:
		/* *** Можно переопределить в потомках *** */

		// Вызывается при успешном завершении обработки элемента при асинхронной обработке.
		// Если элемент обработан успешно (Status == eItemReady), вызывается OnItemReady
		virtual void OnItemReady(T& pItem, LONG_PTR lParam)
		{
			return;
		};
		// Иначе (Status == eItemFailed) - OnItemFailed
		virtual void OnItemFailed(T& pItem, LONG_PTR lParam)
		{
			return;
		};
		// После завершения этих функций ячейка стирается!

	protected:
		/* *** Можно переопределить в потомках *** */

		// Если требуется останов всех запросов и выход из нити обработчика
		virtual bool IsTerminationRequested()
		{
			return mb_TerminateRequested;
		};
		// Здесь потомок может выполнить CoInitialize например
		virtual HRESULT OnThreadStarted()
		{
			return S_OK;
		}
		// Здесь потомок может выполнить CoUninitialize например
		virtual void OnThreadStopped()
		{
			return;
		};
		// Если требуются специфические действия по копированию элемента - переопределить
		virtual void CopyItem(const T* pSrc, T* pDst)
		{
			if (pSrc != pDst)
				memmove(pDst, pSrc, sizeof(T));
		};
		// Если элемент уже был запрошен с другими параметрами, а сейчас пришел новый запрос
		virtual void ApplyChanges(T& pDst, const T& pSrc)
		{
			return;
		}
		// Можно переопределить для изменения логики сравнения (используется при поиске)
		virtual bool IsEqual(const T& pItem1, LONG_PTR lParam1, const T& pItem2, LONG_PTR lParam2)
		{
			int i = memcmp(pItem1, pItem2, sizeof(T));
			return (i == 0) && (lParam1 == lParam2);
		};
		// Если в T определены какие-то указатели - освободить их
		virtual void FreeItem(const T& pItem)
		{
			return;
		};
		// Если элемент потерял актуальность - стал НЕ высокоприоритетным
		virtual bool CheckHighPriority(const T& pItem)
		{
			// Перекрыть в потомке и вернуть false, если, например, был запрос
			// для текущей картинки, но пользователь уже улистал с нее на другую
			return true;
		};

	protected:
		/* *** Эти функции не переопределяются *** */
		HRESULT ProcessItem(ProcessingItem* p)
		{
			HRESULT hr = E_FAIL;
			TRY
			{
				// Собственно, обработка. Выполняется в потомке
				hr = ProcessItem(p->Item, p->lParam);
			}
			CATCH
			{
				hr = E_UNEXPECTED;
			}
			return hr;
		}
		bool ProcessingStep()
		{
			// мы живы
			mn_AliveTick = GetTickCount();
			SetEvent(mh_Alive);
			// Кого будем обрабатывать
			ProcessingItem* p = NULL;

			if (WaitForSingleObject(mh_Waiting, 50) == WAIT_TIMEOUT)
			{
				//_ASSERTE(mn_WaitingCount==0);
				if (mn_WaitingCount == 0)
					return false; // true==Terminate
			}

			// Для получения следующего элемента - нужно заблокировать секцию
			MSectionLockSimple CS;
			CS.Lock(mpcs_Queue);

			// Следующий?
			if (mp_SynchRequest == NULL)
			{
				// Найти требующуюся ячейку
				for (int piority = ePriorityHighest; piority <= ePriorityLowest; piority++)
				{
					for (int i = 0; i < mn_Count; i++)
					{
						if (piority <= ePriorityAboveNormal && mpp_Queue[i]->Priority <= piority)
						{
							if (!CheckHighPriority(mpp_Queue[i]->Item))
							{
								// элемент перестал быть высокоприоритетным
								mpp_Queue[i]->Priority = ePriorityNormal;
								continue;
							}
						}

						if (mpp_Queue[i]->Status == eItemRequest  // Найти "запрашиваемый"
						        // с соответствующим приоритетом
						        && (mpp_Queue[i]->Priority == piority || piority == ePriorityLowest))
						{
							p = mpp_Queue[i]; break;
						}
					}

					// Если нашли - сразу выйдем
					if (p) break;
				}

				// Может этот элемент уже попросили пропустить?
				if (p && p->StopRequested)
				{
					p->StopRequested = false;
					p = NULL;
				}
			}

			// Может пока искали пришел "синхронный" запрос?
			if (mp_SynchRequest)
			{
				p = mp_SynchRequest;
				mp_SynchRequest = NULL; // сразу сбросить, больше не нужен
			}

			if (p)  // Сразу установим статус, до выхода из секции
			{
				p->Status = eItemProcessing;
			}

			CheckWaitingCount();
			// Секция больше не нужна
			CS.Unlock();

			// Может уже был запрос на Terminate?
			if (IsTerminationRequested())
				return true;

			// Если есть, кого обработать
			if (p)
			{
				HRESULT hr = E_FAIL;
				// Чтобы знать, что обрабатывалось
				mp_Active = p;
				// Выставить флаги, что этот элемент "начат" и когда
				p->Start = GetTickCount();
				p->Status = eItemProcessing;
				hr = ProcessItem(p);
				_ASSERTE(hr!=E_NOINTERFACE);
				p->Result = hr;
				p->Status = (hr == S_OK) ? eItemReady : eItemFailed;

				// Сигнализация о "готовности", если запрос был асинхронный
				if (!p->Synch)
				{
					// Вернуть результат обработки (данные)
					if (hr == S_OK)
						OnItemReady(p->Item, p->lParam);
					else
						OnItemFailed(p->Item, p->lParam);

					// И сброс нашей внутренней ячейки
					memset(&p->Item, 0, sizeof(p->Item));
					p->Status = eItemPassed;
				}

				//if (FAILED(hr)) -- не будем так жестоко. Если нужно - позовут RequestTerminate.
				//{
				//	_ASSERTE(SUCCEEDED(hr));
				//	RequestTerminate();
				//	return true;
				//}
			}

			return IsTerminationRequested();
		};
		static DWORD CALLBACK ProcessingThread(LPVOID pParam)
		{
			CQueueProcessor<T>* pThis = (CQueueProcessor<T>*)pParam;

			while(!pThis->IsTerminationRequested())
			{
				if (pThis->ProcessingStep())
					break;
			}

			return 0;
		};
		HRESULT CheckThread()
		{
			// Может нить уже запустили?
			if (mh_Queue && mn_QueueId)
			{
				if (WaitForSingleObject(mh_Queue, 0) == WAIT_OBJECT_0)
				{
					CloseHandle(mh_Queue);
					mh_Queue = NULL;
				}
			}

			if (!mh_Alive)
			{
				mh_Alive = CreateEvent(NULL, FALSE, FALSE, NULL);
				mn_AliveTick = 0;
			}

			if (!mh_Waiting)
			{
				mh_Waiting = CreateEvent(NULL, TRUE, FALSE, NULL);
			}

			if (!mh_Queue)
			{
				mb_TerminateRequested = false;
				ResetEvent(mh_Alive);
				mh_Queue = apiCreateThread(ProcessingThread, this, &mn_QueueId, "Thumbs::ProcessingThread");
			}

			return (mh_Queue == NULL) ? E_UNEXPECTED : S_OK;
		};
};
