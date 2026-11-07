# API визуализатора

[Полный минимальный пример и порядок работы](../README.md#использование-api)
находятся в README. Все типы ниже принадлежат пространству имён `visualizer`.
Примеры в этом справочнике расширяют сцену из минимального примера.

## 1. Сцена и объекты

Основной заголовок: `<scene/scene.hpp>`.

| Вызов | Результат |
|---|---|
| `scene.addObject(object)` | Сохраняет объект по значению, возвращает `ObjectId` |
| `scene.object(id)` | Возвращает ссылку на объект для чтения или изменения |
| `scene.removeObject(id)` | Удаляет объект |
| `scene.objects()` | Даёт доступ к коллекции объектов для чтения |
| `scene.addCamera(camera)` | Сохраняет камеру по значению, возвращает `CameraId` |
| `scene.camera(id)` | Возвращает ссылку на камеру |
| `scene.removeCamera(id)` | Удаляет камеру; её не должен использовать зарегистрированный view |

`SceneObject` объединяет три части:

| Поле | Содержимое |
|---|---|
| `geometry` | Массив вершин и топология: `POINTS`, `LINES`, `TRIANGLES` |
| `material` | Цвет RGBA и флаг `lit`, включающий учёт света |
| `transform` | Позиция, поворот-кватернион и масштаб |

Для `TRIANGLES` каждая последовательная тройка вершин образует треугольник,
для `LINES` — каждая пара образует отрезок, для `POINTS` — каждая вершина точку.
`Vertex` содержит `position` и `normal`, обе — массивы из трёх координат.

Пример добавления и последующего изменения объекта:

```cpp
visualizer::SceneObject object;
object.geometry.vertices = {
    {{-1, -1, 0}, {0, 0, 1}},
    {{1, -1, 0}, {0, 0, 1}},
    {{0, 1, 0}, {0, 0, 1}},
};
const auto objectId = scene.addObject(std::move(object));

auto& storedObject = scene.object(objectId);
storedObject.transform.position = {2, 0, 0};
storedObject.material.color = {0.2f, 0.7f, 1.0f, 1.0f};
storedObject.material.lit = false;
```

После `std::move(object)` изменяй сохранённый объект через `scene.object(id)`.
Линии и точки рисуются без освещения. Для треугольников `lit = false`
позволяет отображать цвет независимо от источников света.

## 2. Камера

Заголовок: `<scene/camera.hpp>`.

`Camera` хранит положение, поворот и настройки проекции. По умолчанию
используется перспективная проекция. Начальное направление взгляда — −Z,
направление вверх — +Y.

| Метод | Назначение |
|---|---|
| `setPosition({x, y, z})` | Положение камеры в мировых координатах |
| `setRotation(quaternion)` | Поворот камеры |
| `setSettings(settings)` | Настройки проекции |
| `position()`, `rotation()`, `settings()` | Чтение текущих параметров |

Переключить камеру сцены на ортографическую проекцию:

```cpp
auto& camera = scene.camera(cameraId);
auto settings = camera.settings();
settings.type = visualizer::Camera::Type::ORTHOGRAPHIC;
settings.verticalSize = 6.0f;
camera.setSettings(settings);
```

`verticalSize` задаёт видимую высоту в единицах сцены. Для перспективы
используются `type = PERSPECTIVE` и `verticalFovRadians` — вертикальный угол
обзора в радианах. `nearPlane` и `farPlane` задают ближнюю и дальнюю границы.
Соотношение сторон вычисляется из размера области просмотра.

## 3. Освещение

Заголовки: `<scene/lightning/lighting.hpp>` и `<scene/lightning/light.hpp>`.

Источники находятся в `scene.lighting`. Новая сцена содержит белый фоновый
свет интенсивностью 0.25 и белый направленный свет интенсивностью 0.75.
Чтобы задать собственное освещение, очисти коллекцию и добавь источники:

```cpp
scene.lighting.clear();

auto ambient = std::make_unique<visualizer::AmbientLight>();
ambient->intensity = 0.2f;
scene.lighting.addLight(std::move(ambient));

auto directional = std::make_unique<visualizer::DirectionalLight>();
directional->towardLight = {0.4f, 0.7f, 1.0f};
directional->intensity = 0.8f;
const auto lightId = scene.lighting.addLight(std::move(directional));

scene.lighting.light(lightId).enabled = false;
```

`Lighting` принимает источники через `std::unique_ptr` и владеет ими.
Общие параметры — `color`, `intensity`, `enabled`. У направленного источника
`towardLight` — мировое направление **от поверхности к источнику**, которое
не обязано иметь единичную длину. Удаление выполняется через `removeLight(id)`.
Пустая коллекция не даёт освещения материалам с `lit = true`.

## 4. Backend и Viewer

Заголовки: `<core/backendFactory.hpp>`, `<core/backendSettings.hpp>`,
`<core/visualizer.hpp>`.

```cpp
visualizer::BackendSettings settings;
settings.window.title = "My scene";
settings.window.width = 1280;
settings.window.height = 720;

auto backend = visualizer::BackendFactory::create(settings);
visualizer::Viewer viewer(std::move(backend));
```

Фабрика создаёт окно GLFW и рендерер OpenGL. Параметры окна также включают
`resizable` и `visible`. Backend выбирается при создании `Viewer` и остаётся
его backend на всё время жизни. После передачи через `std::move` управление
окном и рендерером находится у `Viewer`.

## 5. Область просмотра и контроллер

Заголовки: `<core/viewport.hpp>` и
`<cameraController/orbitCameraController.hpp>`.

```cpp
viewer.addView(scene, cameraId, viewport, std::move(controller));
```

| Аргумент | Назначение |
|---|---|
| `scene` | Сцена, которая будет отображаться |
| `cameraId` | Камера из этой сцены |
| `viewport` | Область окна для отображения |
| `controller` | Необязательный контроллер камеры, передаваемый во владение |

`NormalizedViewport` содержит `x`, `y`, `width`, `height`. Координаты
отсчитываются от левого верхнего угла окна; значения нормализованы от 0 до 1.
Ширина и высота положительны, вся область должна помещаться в окно.
По умолчанию `{0, 0, 1, 1}` занимает всё окно.

Если управление мышью не нужно, достаточно:

```cpp
viewer.addView(scene, cameraId);
```

Несколько областей можно зарегистрировать до запуска цикла. Этот пример
**заменяет** единственный `addView` из минимального примера:

```cpp
visualizer::Camera secondCamera;
secondCamera.setPosition({0, 0, 8});
const auto secondCameraId = scene.addCamera(std::move(secondCamera));

const visualizer::NormalizedViewport leftHalf{0, 0, 0.5f, 1};
const visualizer::NormalizedViewport rightHalf{0.5f, 0, 0.5f, 1};

viewer.addView(scene, cameraId, leftHalf,
               std::make_unique<visualizer::OrbitCameraController>());
viewer.addView(scene, secondCameraId, rightHalf,
               std::make_unique<visualizer::OrbitCameraController>());
```

Области не должны перекрываться. Одна камера в одном `Viewer` может иметь
только один контроллер; при этом показывать её в нескольких областях без
дополнительных контроллеров разрешено. Контроллер получает ввод выбранной
области. `OrbitCameraController` вращает камеру вокруг целевой точки,
перемещает эту точку и меняет расстояние до неё.

## 6. Цикл отображения

Для обычного приложения вызови `viewer.run()`. Метод сам опрашивает события
окна, вычисляет время кадра, обновляет управление и отрисовывает сцену.
Он возвращает управление после закрытия окна.

Для собственного цикла доступны:

| Метод | Что делает |
|---|---|
| `viewer.update(deltaTime)` | Получает накопленный ввод и обновляет контроллеры; время задаётся в секундах |
| `viewer.drawFrame()` | Рисует зарегистрированные области, используя текущие сцены и камеры |

В собственном цикле нужно самостоятельно вызывать `Window::pollEvents()`
перед `update`, проверять `Window::shouldClose()` и вычислять время кадра.
Доступ к окну можно сохранить через `backend.window.get()` **до** передачи
backend в `Viewer`. Указатель действителен, пока окно принадлежит живому
`Viewer`. `update` и `drawFrame` сами события окна не опрашивают.

Пример такого цикла есть в функции `runDemo` в
[`examples/renderTest.cpp`](../examples/renderTest.cpp).
